# ===================================================================
# CBGPValid:Topologies.pm
#
# author Bruno Quoitin (bruno.quoitin@umons.ac.be)
# $Id: Topologies.pm,v 1.2 2009-06-25 14:36:27 bqu Exp $
# ===================================================================

package CBGPValid::Topologies;

require Exporter;
use CBGPValid::UI;

@ISA= qw(Exporter);
@EXPORT= qw(
	    F_TOPO_LINK_DELAY
	    F_TOPO_LINK_WEIGHT

	    C_TOPO_ADDR_SCH_LOCAL
	    C_TOPO_ADDR_SCH_DEFAULT
	    C_TOPO_ADDR_SCH_ASN

	    topo_2nodes
	    topo_3nodes_triangle
	    topo_3nodes_line
	    topo_star
	    topo_get_nodes
	    topo_included

	    cbgp_topo

	    topo_from_caida
	    topo_from_ntf
	    topo_from_subramanian_file
	    topo_from_subramanian_array
	    topo_from_subramanian_line
	    topo_check_asn
	    topo_check_relation

	    set_tests
	   );

use strict;
use CBGPValid::Functions;

use constant F_TOPO_LINK_DELAY  => 0;
use constant F_TOPO_LINK_WEIGHT => 1;

use constant C_TOPO_ADDR_SCH_DEFAULT => "default";
use constant C_TOPO_ADDR_SCH_LOCAL   => "local";
use constant C_TOPO_ADDR_SCH_ASN     => "asn";

my $tests;
1;

# -----[ set_tests ]-------------------------------------------------
sub set_tests($) {
  ($tests)= @_;
}

# -----[ topo_2nodes ]-----------------------------------------------
sub topo_2nodes()
  {
    my %topo;
    $topo{'1.0.0.1'}{'1.0.0.2'}= [10, 10];
    return \%topo;
  }

# -----[ topo_3nodes_triangle ]--------------------------------------
sub topo_3nodes_triangle()
  {
    my %topo;
    $topo{'1.0.0.1'}{'1.0.0.2'}= [10, 10];
    $topo{'1.0.0.1'}{'1.0.0.3'}= [10, 5];
    $topo{'1.0.0.2'}{'1.0.0.3'}= [10, 4];
    return \%topo;
  }

# -----[ topo_3nodes_line ]------------------------------------------
sub topo_3nodes_line()
  {
    my %topo;
    $topo{'1.0.0.1'}{'1.0.0.2'}= [10, 10];
    $topo{'1.0.0.2'}{'1.0.0.3'}= [10, 10];
    return \%topo;
  }

# -----[ topo_star ]-------------------------------------------------
sub topo_star($)
  {
    my ($num_nodes)= @_;
    my %topo;

    for (my $index= 0; $index < $num_nodes-1; $index++) {
      $topo{'1.0.0.1'}{'1.0.0.'.($index+2)}= [10, 10];
    }
    return \%topo;
  }

# -----[ topo_get_nodes ]--------------------------------------------
sub topo_get_nodes($)
  {
    my ($topo)= @_;
    my %nodes;

    foreach my $node1 (keys %$topo) {
      foreach my $node2 (keys %{$topo->{$node1}}) {
	(!exists($nodes{$node1})) and $nodes{$node1}= 1;
	(!exists($nodes{$node2})) and $nodes{$node2}= 1;
      }
    }
    return \%nodes;
  }

# -----[ topo_included ]---------------------------------------------
sub topo_included($$)
  {
    my ($topo1, $topo2)= @_;

    foreach my $node1 (keys %$topo1) {
      foreach my $node2 (keys %{$topo1->{$node1}}) {
	my $link1= $topo1->{$node1}{$node2};
	my $link2;
	if (exists($topo2->{$node1}{$node2})) {
	  $link2= $topo2->{$node1}{$node2};
	} elsif (exists($topo2->{$node2}{$node1})) {
	  $link2= $topo2->{$node2}{$node1};
	}
	if (!defined($link2)) {
	  $tests->debug("missing link ($node1, $node2)\n");
	  return 0;
	}
	if (@$link1 != @$link2) {
	  $tests->debug("incoherent number of attributes ".
			"($node1 [".@$link1."], $node2 [".@$link2."])\n");
	  return 0;
	}
	for (my $index= 0; $index < @$link1; $index++) {
	  if ($link1->[$index] != $link2->[$index]) {
	    $tests->debug("incoherent attribute [$index] ".
			  "($node1, $node2): $link1->[$index] vs ".
			  "$link2->[$index]\n");
	    return 0;
	  }
	}
      }
    }
    return 1;
  }


#####################################################################
#
# Build C-BGP model from topology data structure
#
#####################################################################

# -----[ cbgp_topo ]-------------------------------------------------
sub cbgp_topo($$;$) {
  my ($cbgp, $topo, $domain)= @_;
  my %nodes;

  if (defined($domain)) {
    $cbgp->send_cmd("net add domain $domain igp");
  }

  foreach my $node1 (keys %$topo) {
    foreach my $node2 (keys %{$topo->{$node1}}) {
      my ($delay, $weight)= @{$topo->{$node1}{$node2}};
      if (!exists($nodes{$node1})) {
	$cbgp->send_cmd("net add node $node1");
	if (defined($domain)) {
	  $cbgp->send_cmd("net node $node1 domain $domain");
	}
	$nodes{$node1}= 1;
      }
      if (!exists($nodes{$node2})) {
	$cbgp->send_cmd("net add node $node2");
	if (defined($domain)) {
	  $cbgp->send_cmd("net node $node2 domain $domain");
	}
	$nodes{$node2}= 1;
      }
      $cbgp->send_cmd("net add link $node1 $node2 --delay=$delay");
      $cbgp->send_cmd("net link $node1 $node2 igp-weight $weight");
      $cbgp->send_cmd("net link $node2 $node1 igp-weight $weight");
    }
  }
}


#####################################################################
#
# Load topologies from known formats
#
#####################################################################

# -----[ topo_from_caida ]-------------------------------------------
# Load a topology from an AS-level topology (in "CAIDA" format).
# The resulting topology contains one router/AS. The IP address of
# each router is ASx.ASy.0.0
# Each link is defined in both directions.
# -------------------------------------------------------------------
sub topo_from_caida($;$)
  {
    my ($filename, $addr_sch)= @_;
    my %topo;

    if (!open(CAIDA, "<$filename")) {
      show_error("unable to open \"$filename\": $!");
      exit(-1);
    }
    while (<CAIDA>) {
      chomp;
      (m/^\#/) and next;
      my @fields= split /\s+/;
      if (@fields != 3) {
	show_error("invalid caida record");
	exit(-1);
      }
      my ($ip0, $ip1);
      if (!defined($addr_sch) || ($addr_sch eq "default")) {
	$ip0= int($fields[0] / 256).".".($fields[0] % 256).".0.0";
	$ip1= int($fields[1] / 256).".".($fields[1] % 256).".0.0";
      } elsif ($addr_sch eq "local") {
	$ip0= "0.0.".int($fields[0] / 256).".".($fields[0] % 256);
	$ip1= "0.0.".int($fields[1] / 256).".".($fields[1] % 256);
      } else {
	die;
      }
      $topo{$ip0}{$ip1}= [0, 0];
    }
    close(CAIDA);

    return \%topo;
  }

# -----[ topo_from_ntf ]---------------------------------------------
# Load a topology from a NTF file.
# -------------------------------------------------------------------
sub topo_from_ntf($)
  {
    my ($ntf_file)= @_;
    my %topo;

    if (!open(NTF, "<$ntf_file")) {
      show_error("unable to open \"$ntf_file\": $!");
      exit(-1);
    }
    while (<NTF>) {
      chomp;
      (m/^\#/) and next;
      my @fields= split /\s+/;
      if (@fields < 3) {
	show_error("invalid NTF record");
	exit(-1);
      }
      my @attr= ();
      $attr[F_TOPO_LINK_DELAY]= 0;
      $attr[F_TOPO_LINK_WEIGHT]= $fields[2];
      if (@fields > 3) {
	$attr[F_TOPO_LINK_DELAY]= $fields[3];
      }
      $topo{$fields[0]}{$fields[1]}= \@attr;
    }
    close(NTF);

    return \%topo;
  }

# -----[ topo_from_subramanian_line ]--------------------------------
# This function is responsible for parsing a single line of AS-level
# topology in the "Subramanian" format. If the line can be parsed
# the relationship is added to the referenced topology.
# -------------------------------------------------------------------
sub topo_from_subramanian_line($$;$)
{
  my ($topo, $line, $addr_sch)= @_;

  # Skip comments
  ($line =~ m/^\#/) and next;

  # Split in fields
  my @fields= split /\s+/, $line;
  if (@fields != 3) {
    show_error("invalid subramanian record");
    exit(-1);
  }

  # Convert ASN to IP address according to addressing scheme
  my ($ip0, $ip1);
  if (!defined($addr_sch) || ($addr_sch eq C_TOPO_ADDR_SCH_DEFAULT)) {
    $ip0= int($fields[0] / 256).".".($fields[0] % 256).".0.0";
    $ip1= int($fields[1] / 256).".".($fields[1] % 256).".0.0";
  } elsif ($addr_sch eq C_TOPO_ADDR_SCH_LOCAL) {
    $ip0= "0.0.".int($fields[0] / 256).".".($fields[0] % 256);
    $ip1= "0.0.".int($fields[1] / 256).".".($fields[1] % 256);
  } elsif ($addr_sch eq C_TOPO_ADDR_SCH_ASN) {
    $ip0= $fields[0];
    $ip1= $fields[1];
  } else {
    die "invalid addr-sch option \"$addr_sch\"";
  }

  # Add relationship to topology data structure
  $topo->{$ip0}{$ip1}= [0, 0];
  $topo->{$ip1}{$ip0}= [0, 0];
}

# -----[ topo_from_subramanian_file ]--------------------------------
# Load a topology from an AS-level topology (in "Subramanian" format).
# The resulting topology contains one router/AS. The IP address of
# each router is ASx.ASy.0.0
# -------------------------------------------------------------------
sub topo_from_subramanian_file($;$) {
  my ($filename, $addr_sch)= @_;
  my %topo;

  if (!open(SUBRAMANIAN, "<$filename")) {
    show_error("unable to open \"$filename\": $!");
    exit(-1);
  }
  while (my $line= <SUBRAMANIAN>) {
    chomp $line;
    topo_from_subramanian_line(\%topo, $line, $addr_sch);
  }
  close(SUBRAMANIAN);
  return \%topo;
}

# -----[ topo_from_subramanian_array ]-------------------------------
# Load a topology from an AS-level topology (in "Subramanian" format).
# The topology description is obtained from an array.
# -------------------------------------------------------------------
sub topo_from_subramanian_array($;$) {
  my ($array, $addr_sch)= @_;
  my %topo;

  foreach my $line (@$array) {
    topo_from_subramanian_line(\%topo, $line, $addr_sch);
  }
  return \%topo;
}

# -----[ topo_check_asn ]--------------------------------------------
# Check that the given ASN is present in the topology data structure.
# -------------------------------------------------------------------
sub topo_check_asn($$) {
  my ($topo, $asn)= @_;
  $tests->debug("checking presence of $asn");
  return 0
    if (!exists($topo->{$asn}));
  return 1;
}

# -----[ topo_check_relation ]---------------------------------------
# Check that a relationship exists between two ASNs in the topology
# data structure.
# -------------------------------------------------------------------
sub topo_check_relation($$$) {
  my ($topo, $asn1, $asn2)= @_;
  $tests->debug("checking presence of relation $asn1-$asn2");
  return 0
    if (!exists($topo->{$asn1}) ||
	!exists($topo->{$asn1}{$asn2}));
  return 1;
}
