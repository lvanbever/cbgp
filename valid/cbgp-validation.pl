#!perl -w
# ===================================================================
# @(#)cbgp-validation.pl
#
# This script is used to validate C-BGP. It performs various tests in
# order to detect erroneous behaviour.
#
# @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
# $Id: cbgp-validation.pl,v 1.47 2009-08-31 09:49:12 bqu Exp $
# ===================================================================
# Syntax:
#
#   ./cbgp-validation.pl [OPTIONS]
#
# where possible options are
#
#   --cbgp-path=PATH
#                   Specifies the path of the C-BGP binary. The
#                   default path is "../src/valid"
#
#   --no-cache
#                   The default behaviour of the validation script is
#                   to store (cache) the test results. When a test
#                   was succesfull, it is not executed again, unless
#                   the --no-cache argument is passed.
#
#  --include=TEST
#                   Specifies a TEST to include in the validation.
#                   This argument can be mentionned multiple times in
#                   order to include multiple tests. Note that a test
#                   is executed only once, regardless of the number
#                   of times it appears with --include. The default
#                   behaviour is to execute all the tests. If at
#                   least one --include argument is passed, the
#                   script will only execute the explicitly specified
#                   tests.
#
# --report=FORMAT
#                   Generates a report summarizing the tests results.
#                   The script currently only support HTML and XML
#                   formats, so use --report=html or --report=xml.
#                   The report is composed of two files/pages. The
#                   first one contains a summary table with all tests
#                   results. The second one contains the description
#                   of each test. The description is automatically
#                   extracted from the test Perl functions.
#
# --report-prefix=PREFIX
#                   Changes the prefix of the report file names. The
#                   result page will be called "PREFIX.html" or
#                   "PREFIX.xml" (depending on the reports format)
#                   while the documentation page will be called
#                   "PREFIX-doc.html" or "PREFIX-doc.xml".
#
# --resources-path=PATH
#                   Set the path to the resources needed by the
#                   validation script. The needed resources are the
#                   topology files, the BGP RIB dumps, and so on.
#                   Usually, this path must be set to the installation
#                   directory of the validation script.
#
# ===================================================================
# Contributions:
#   - Sebastien Tandel: record-route loop (March 2007)
# ===================================================================

use strict;
use warnings;

use Getopt::Long;
use CBGP;
use CBGPValid::BaseReport;
use CBGPValid::Checking;
use CBGPValid::Functions;
use CBGPValid::HTMLReport;
use CBGPValid::TestConstants;
use CBGPValid::Tests;
use CBGPValid::Topologies;
use CBGPValid::UI;
use CBGPValid::XMLReport;
use POSIX;


use constant CBGP_VALIDATION_VERSION => '1.13';

# -----[ Error messages ]-----
my $CBGP_ERROR_INVALID_SUBNET    = "invalid subnet";
my $CBGP_ERROR_CLUSTERID_INVALID = "invalid cluster-id";
my $CBGP_ERROR_DOMAIN_UNKNOWN    = "unknown domain";
my $CBGP_ERROR_LINK_EXISTS       = "could not add link";
my $CBGP_ERROR_LINK_ENDPOINTS    = "link endpoints are equal";
my $CBGP_ERROR_INVALID_LINK_TAIL = "tail-end .* does not exist";
my $CBGP_ERROR_NODE_EXISTS       = "node already exists";
my $CBGP_ERROR_ROUTE_BAD_IFACE   = "interface is unknown";
my $CBGP_ERROR_ROUTE_EXISTS      = "route already exists";
my $CBGP_ERROR_ROUTE_NH_UNREACH  = "next-hop is unreachable";
my $CBGP_ERROR_ROUTEMAP_EXISTS   = "route-map already exists";

use constant MAX_IGP_WEIGHT => 4294967295;

use constant MRT_PREFIX => 5;
use constant MRT_NEXTHOP => 8;

# -----[ Expected C-BGP version ]-----
# These are the expected major/minor versions of the C-BGP instance
# we are going to test.If these versions are lower than reported by
# the C-BGP instance, a warning will be issued.
use constant CBGP_MAJOR_MIN => 1;
use constant CBGP_MINOR_MIN => 4;

use constant DEFAULT_REPORT_PREFIX => "cbgp-valid";

# -----[ Command-line options ]-----
my $max_failures= 0;
my $max_warnings= 0;
my $report_prefix= DEFAULT_REPORT_PREFIX;

my $validation= {
		 'cbgp_version'    => undef,
		 'libgds_version'  => undef,
		 'program_args'    => (join " ", @ARGV),
		 'program_name'    => $0,
		 'program_version' => CBGP_VALIDATION_VERSION,
		 'resources_path'  => '.',
		 'tmp_path'        => '/tmp',
		};

show_info("c-bgp CLI validation v".$validation->{'program_version'});
show_info("(c) 2008-2013, Bruno Quoitin (bruno.quoitin\@umons.ac.be)");

my %opts;
if (!GetOptions(\%opts,
		"cbgp-path:s",
		"cache!",
		"debug!",
		"glibtool!",
		"help!",
		"libtool!",
		"max-failures:i",
		"max-warnings:i",
		"include:s@",
		"report:s",
	        "report-prefix:s",
		"resources-path:s",
	        "valgrind!")) {
  show_error("Invalid command-line options");
  exit(-1);
}

if (exists($opts{'help'})) {
  help();
  exit;
}

if (exists($opts{'cache'}) && !$opts{'cache'}) {
  $opts{'cache'}= 0;
} else {
  $opts{'cache'}= 1;
}

(!exists($opts{'cbgp-path'})) and
  $opts{'cbgp-path'}= "../src/cbgp";
(exists($opts{'resources-path'})) and
  $validation->{'resources_path'}= $opts{'resources-path'};
($validation->{'resources_path'} =~ s/^(.*[^\/])$/$1\//);
(exists($opts{"max-failures"})) and
  $max_failures= $opts{"max-failures"};
(exists($opts{"max-warnings"})) and
  $max_failures= $opts{"max-warnings"};
(exists($opts{"report-prefix"})) and
  $report_prefix= $opts{"report-prefix"};

# -----[ Validation setup parameters ]-----
my $tests= CBGPValid::Tests->new(-debug=>$opts{'debug'},
				 -cache=>$opts{'cache'},
				 -cbgppath=>$opts{'cbgp-path'},
				 -include=>$opts{'include'},
				 -maxfailures=>$max_failures,
				 -valgrind=>$opts{'valgrind'},
				 -libtool=>$opts{'libtool'},
				 -glibtool=>$opts{'glibtool'});
CBGPValid::Checking::set_tests($tests);
CBGPValid::Topologies::set_tests($tests);


# -----[ help ]------------------------------------------------------
#
# -------------------------------------------------------------------
sub help() {
  print "\n";
  print "Usage:\n";
  print "  cbgp-validation.pl\n";
  print "    [--cache|--no-cache]\n";
  print "    [--cbgp-path=PATH]\n";
  print "    [--debug]\n";
  print "    [--help]\n";
  print "    [--include=NAME]\n";
  print "    [--max-failures=MAX]\n";
  print "    [--max-warnings=MAX]\n";
  print "    [--report=TYPE]\n";
  print "    [--report-prefix=PREFIX]\n";
  print "    [--resources-path=PATH]\n";
  print "\n";
}

# -----[ get_resource ]----------------------------------------------
#
# -------------------------------------------------------------------
sub get_resource($) {
  my ($filename)= @_;
  return $validation->{'resources_path'}.'/'.$filename;
}

# -----[ get_tmp_resource ]------------------------------------------
#
# -------------------------------------------------------------------
sub get_tmp_resource($) {
  my ($filename)= @_;
  return $validation->{'tmp_path'}.'/'.$filename;
}

# -----[ canonic_prefix ]--------------------------------------------
#
# -------------------------------------------------------------------
sub canonic_prefix($)
  {
    my $prefix= shift;
    my ($network, $mask);

    if ($prefix =~ m/^([0-9.]+)\/([0-9]+)$/) {
      $network= $1;
      $mask= $2;
    } else {
      die "Error: invalid prefix [$prefix]";
    }

    my @fields= split /\./, $network;
    my $network_int= (($fields[0]*256+
		       (defined($fields[1])?$fields[1]:0))*256+
		      (defined($fields[2])?$fields[2]:0))*256+
			(defined($fields[3])?$fields[3]:0);
    $network_int= $network_int >> (32-$mask);
    $network_int= $network_int << (32-$mask);
    return "".(($network_int >> 24) & 255).".".
      (($network_int >> 16) & 255).".".
	(($network_int >> 8) & 255).".".
	  ($network_int & 255)."/$mask";
  }

# -----[ cbgp_check_error ]------------------------------------------
# Check if C-BGP has returned an error message.
#
# Return values:
#   undef      -> no error message was issued
#   <message>  -> error message returned by C-BGP
# -------------------------------------------------------------------
sub cbgp_check_error($$) {
  my ($cbgp, $cmd)= @_;
  my $error= undef;
  my $reason= undef;

  $cbgp->send_cmd("set exit-on-error off");
  $cbgp->send_cmd($cmd);
  $cbgp->send_cmd("print \"CHECKPOINT\\n\"");
  while ((my $line= $cbgp->expect(1)) ne "CHECKPOINT") {
    if ($line =~ m/^.*Error\:\ (.+)/) {
      (defined($error)) and die "Duplicate error returned";
      $error= $1;
    } elsif ($line =~ m/^\s\+\-\-\ reason\:\ (.+)$/) {
      (!defined($error)) and die "Reason without error";
      (defined($reason)) and die "Duplicate error reason returned";
      $reason= $1;
    }
  }

  return $reason;
}

# -----[ check_has_error ]-------------------------------------------
sub check_has_error($;$) {
  my ($msg, $error)= @_;

  $tests->debug("check_error(".(defined($error)?$error:"").")");
  if (!defined($msg)) {
    $tests->debug("no error message found.");
    return 0;
  }

  if (defined($error) && !($msg =~ m/$error/)) {
    $tests->debug("error message defined, but does not match.");
    $tests->debug("received=[$msg]");
    $tests->debug("expected=[$error]");
    return 0;
  }
  return 1;
}

# -----[ cbgp_topo_domain ]------------------------------------------
sub cbgp_topo_domain($$$$)
  {
    my ($cbgp, $topo, $predicate, $domain)= @_;

    $cbgp->send_cmd("net add domain $domain igp");

    my $nodes= topo_get_nodes($topo);
    foreach my $node (keys %$nodes) {
      if ($node =~ m/$predicate/) {
	$cbgp->send_cmd("net node $node domain $domain");
      }
    }
  }

# -----[ cbgp_topo_filter ]------------------------------------------
# Create a simple topology suitable for testing filters.
#
# Setup:
#   - R1 (1.0.0.1) in AS1
#   - R2 (1.0.0.2) in AS2 virtual peer
# -------------------------------------------------------------------
sub cbgp_topo_filter($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1");
    $cbgp->send_cmd("net node 1.0.0.1 route add --oif=2.0.0.1 2.0.0.1/32 0");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "virtual");
  }

# -----[ cbgp_topo_dp ]----------------------------------------------
# Create a simple topology suitable for testing the decision
# process. All peers are in different ASs.
#
# Setup:
#   - R1 (1.0.0.1) in AS1
#   - R2 (2.0.0.1) in AS2 virtual peer
#   - ...
#   - R[n+1] ([n+1].0.0.1) in AS[n+1] virtual peer
# -------------------------------------------------------------------
sub cbgp_topo_dp($$)
  {
    my ($cbgp, $num_peers)= @_;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < $num_peers; $index++) {
      my $asn= $index+2;
      my $peer= "$asn.0.0.1";
      $cbgp->send_cmd("net add node $peer");
      $cbgp->send_cmd("net add link 1.0.0.1 $peer");
      $cbgp->send_cmd("net node 1.0.0.1 route add --oif=$peer $peer/32 0");
      cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual");
    }
  }

# -----[ cbgp_topo_dp2 ]---------------------------------------------
sub cbgp_topo_dp2($$)
  {
    my ($cbgp, $num_peers)= @_;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < $num_peers; $index++) {
      my $peer= "1.0.0.".($index+2);
      $cbgp->send_cmd("net add node $peer");
      $cbgp->send_cmd("net add link 1.0.0.1 $peer");
      $cbgp->send_cmd("net node 1.0.0.1 route add --oif=$peer $peer/32 0");
      cbgp_peering($cbgp, "1.0.0.1", $peer, "1", "virtual");
    }
  }

# -----[ cbgp_topo_dp3 ]---------------------------------------------
sub cbgp_topo_dp3($;@)
  {
    my ($cbgp, @peers)= @_;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < @peers; $index++) {
      my ($peer, $asn, $weight)= @{$peers[$index]};
      $cbgp->send_cmd("net add node $peer\n");
      $cbgp->send_cmd("net add link 1.0.0.1 $peer\n");
      $cbgp->send_cmd("net node 1.0.0.1 route add --oif=$peer $peer/32 $weight\n");
      cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual");
    }
  }

# -----[ cbgp_topo_dp4 ]---------------------------------------------
# Create a topology composed of a central router R1 (1.0.0.1, AS1)
# and a set of virtual and non-virtual peers.
#
# The peer specification is as follows:
#   <IP address>, <ASN>, <IGP weight>, <virtual?>, <options>
# -------------------------------------------------------------------
sub cbgp_topo_dp4($;@)
  {
    my ($cbgp, @peers)= @_;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    for (my $index= 0; $index < @peers; $index++) {
      my @peer_record= @{$peers[$index]};
      my $peer= shift @peer_record;
      my $asn= shift @peer_record;
      my $weight= shift @peer_record;
      my $virtual= shift @peer_record;
      $cbgp->send_cmd("net add node $peer");
      $cbgp->send_cmd("net add link 1.0.0.1 $peer");
      $cbgp->send_cmd("net node 1.0.0.1 route add --oif=$peer $peer/32 $weight");
      if ($virtual) {
	cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, "virtual", @peer_record);
      } else {
	$cbgp->send_cmd("net node $peer route add --oif=1.0.0.1 1.0.0.1/32 $weight");
	$cbgp->send_cmd("bgp add router $asn $peer");
	cbgp_peering($cbgp, "1.0.0.1", $peer, $asn, @peer_record);
	cbgp_peering($cbgp, $peer, "1.0.0.1", 1);
      }
    }
  }

# -----[ cbgp_topo_igp_bgp ]-----------------------------------------
# Create a topology along with BGP configuration suitable for testing
# IGP/BGP interaction scenari.
#
# R1.2 --(W2)--\
#               \
# R1.3 --(W3)----*--- R1.1 --- R2.1
#               /
# R2.n --(Wn)--/
#
# AS1: R1.1 - R1.n
# AS2: R2.1
#
# Wi = weight of link
# -------------------------------------------------------------------
sub cbgp_topo_igp_bgp($;@) {
  my $cbgp= shift @_;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");

  foreach my $peer_spec (@_) {
    my $peer_ip= $peer_spec->[0];
    my $peer_weight= $peer_spec->[1];
    $cbgp->send_cmd("net add node $peer_ip");
    $cbgp->send_cmd("net node $peer_ip domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 $peer_ip");
    $cbgp->send_cmd("net link 1.0.0.1 $peer_ip".
		    " igp-weight --bidir $peer_weight");
  }
  $cbgp->send_cmd("net domain 1 compute");

  $cbgp->send_cmd("net add node 2.0.0.1");

  $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 route add --oif=2.0.0.1 2.0.0.1/32 0");
  $cbgp->send_cmd("net node 2.0.0.1 route add --oif=1.0.0.1 1.0.0.1/32 0");

  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  foreach my $peer_spec (@_) {
    my $peer_ip= $peer_spec->[0];
    $cbgp->send_cmd("\tadd peer 1 $peer_ip");
    $cbgp->send_cmd("\tpeer $peer_ip virtual");
    for (my $index= 2; $index < scalar(@$peer_spec); $index++) {
      my $peer_option= $peer_spec->[$index];
      $cbgp->send_cmd("\tpeer $peer_ip $peer_option");
    }
    $cbgp->send_cmd("\tpeer $peer_ip up");
  }
  $cbgp->send_cmd("\tadd peer 2 2.0.0.1");
  $cbgp->send_cmd("\tpeer 2.0.0.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 2 2.0.0.1");
  $cbgp->send_cmd("bgp router 2.0.0.1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\texit");
}

# -----[ cbgp_topo_igp_compute ]-------------------------------------
sub cbgp_topo_igp_compute($$$)
  {
    my ($cbgp, $topo, $domain)= @_;
    $cbgp->send_cmd("net domain $domain compute");
  }

# -----[ cbgp_topo_bgp_routers ]-------------------------------------
sub cbgp_topo_bgp_routers($$$) {
  my ($cbgp, $topo, $asn)= @_;
  my $nodes= topo_get_nodes($topo);

  foreach my $node (keys %$nodes) {
    $cbgp->send_cmd("bgp add router $asn $node");
  }
}

# -----[ cbgp_topo_ibgp_sessions ]-----------------------------------
sub cbgp_topo_ibgp_sessions($$$)
  {
    my ($cbgp, $topo, $asn)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node1 (keys %$nodes) {
      foreach my $node2 (keys %$nodes) {
	($node1 eq $node2) and next;
	$cbgp->send_cmd("bgp router $node1 add peer $asn $node2");
	$cbgp->send_cmd("bgp router $node1 peer $node2 up");
      }
    }
  }

# -----[ cbgp_topo_check_bgp_networks ]------------------------------
sub cbgp_topo_check_bgp_networks($$;$;$)
  {
    my ($cbgp, $topo, $prefixes, $x)= @_;
    my $nodes= topo_get_nodes($topo);

    if (!defined($prefixes)) {
      push @$prefixes, ("255.255.0.0/16");
    }

    foreach my $node1 (keys %$nodes) {

      # Add networks
      foreach my $prefix (@$prefixes) {
	$cbgp->send_cmd("bgp router $node1 add network $prefix");
	$cbgp->send_cmd("sim run");
      }
	
      # Check that the routes are present in each router's RIB
      foreach my $node2 (keys %$nodes) {
	my $rib= cbgp_get_rib($cbgp, $node2);
	if (scalar(keys %$rib) != @$prefixes) {
	  print "invalid number of routes in RIB (".
	    scalar(keys %$rib)."), should be ".@$prefixes."\n";
	  return 0;
	}
	foreach my $prefix (@$prefixes) {
	  if (!exists($rib->{$prefix})) {
	    print "missing prefix $prefix :-(\n";
	    return 0;
	  }
	  # Nexthop should be the origin router
	  if ($rib->{$prefix}->[F_RIB_NEXTHOP] ne $node1) {
	    print "incorrect next-hop :-(\n";
	    return 0;
	  }

	  # Route should be marked as 'best' in all routers
	  if (!($rib->{$prefix}->[F_RIB_STATUS] =~ m/>/)) {
	    print "not best :-(\n";
	    return 0;
	  }

	  if ($node2 eq $node1) {
	    # Route should be marked as 'internal' in the origin router
	    if (!($rib->{$prefix}->[F_RIB_STATUS] =~ m/i/)) {
	      print "not internal :-(\n";
	      return 0;
	    }
	  } else {
	    # Route should be marked as 'reachable' in other routers
	    if (!($rib->{$prefix}->[F_RIB_STATUS] =~ m/\*/)) {
	      print "not reachable :-(\n";
	      return 0;
	    }
	  }
	  # Check that the route is installed into the IP
	  # routing table
	  if ($node1 ne $node2) {
	    my $rt= cbgp_get_rt($cbgp, $node2);
	    if (!exists($rt->{$prefix}) ||
		($rt->{$prefix}->[F_RT_PROTO] ne C_RT_PROTO_BGP)) {
	      print "route not installed in IP routing table\n";
	      return 0;
	    }
	  }
	}
      }	

      # Remove networks
      foreach my $prefix (@$prefixes) {	
	$cbgp->send_cmd("bgp router $node1 del network $prefix");
	$cbgp->send_cmd("sim run");

	# Check that the routes have been withdrawn
	foreach my $node2 (keys %$nodes) {
	  my $rib;
	  # Check that the route is not installed into the Loc-RIB
	  # (best)
	  $rib= cbgp_get_rib($cbgp, $node2);
	  if (scalar(keys %$rib) != 0) {
	    print "invalid number of routes in RIB (".
	      scalar(keys %$rib)."), should be 0\n";
	    return 0;
	  }
	  # Check that the route is not present in the Adj-RIB-in
	  $rib= cbgp_get_rib_in($cbgp, $node2);
	  if (scalar(keys %$rib) != 0) {
	    print "invalid number of routes in Adj-RIB-in (".
	      scalar(keys %$rib)."), should be 0\n";
	  }
	  # Check that the route has been uninstalled from the IP
	  # routing table
	  if ($node1 ne $node2) {
	    my $rt= cbgp_get_rt($cbgp, $node2);
	    if (exists($rt->{$prefix}) &&
		($rt->{$prefix}->[F_RT_PROTO] eq C_RT_PROTO_BGP)) {
	      print "route still installed in IP routing table\n";
	      return 0;
	    }
	  }
	}
      }

    }

    return 1;
  }

# -----[ cbgp_topo_check_igp_reachability ]--------------------------
sub cbgp_topo_check_igp_reachability($$)
  {
    my ($cbgp, $topo)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node1 (keys %$nodes) {
      my $routes= cbgp_get_rt($cbgp, $node1);
      foreach my $node2 (keys %$nodes) {
	($node1 eq $node2) and next;
	if (!exists($routes->{"$node2/32"})) {
	  $tests->debug("no route towards destination ($node2/32)");
	  return 0;
	}
	my $type= $routes->{"$node2/32"}->[F_RT_PROTO];
	if ($type ne C_RT_PROTO_IGP) {
	  $tests->debug("route towards $node2/32 is not from IGP ($type)");
	  return 0;
	}
      }
    }
    return 1;
  }

# -----[ cbgp_topo_check_ibgp_sessions ]-----------------------------
# Check that there is one iBGP session between each pair of nodes in
# the topology (i.e. a full-mesh of iBGP sessions).
#
# Optionally, check that the sessions' state is equal to the given
# state.
# -------------------------------------------------------------------
sub cbgp_topo_check_ibgp_sessions($$;$)
  {
    my ($cbgp, $topo, $state)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node1 (keys %$nodes) {
      my $peers= cbgp_get_peers($cbgp, $node1);
      foreach my $node2 (keys %$nodes) {
	($node1 eq $node2) and next;
	if (!exists($peers->{$node2})) {
	  print "missing session $node1 <-> $node2\n";
	  return 0;
	}
	if (defined($state) &&
	    ($peers->{$node2}->[F_PEER_STATE] ne $state)) {
	  print "$state\t$peers->{$node2}->[F_PEER_STATE]\n";
	  return 0;
	}
      }
    }
    return 1;
  }

# -----[ cbgp_topo_check_ebgp_sessions ]-----------------------------
# Check that there is one eBGP session for each link in the topology.
# Check that sessions are defined in both ways.
#
# Optionally, check that the sessions' state is equal to the given
# state.
# -------------------------------------------------------------------
sub cbgp_topo_check_ebgp_sessions($$;$)
  {
    my ($cbgp, $topo, $state)= @_;

    foreach my $node1 (keys %$topo) {
      my $peers1= cbgp_get_peers($cbgp, $node1);
      foreach my $node2 (keys %{$topo->{$node1}}) {
	my $peers2= cbgp_get_peers($cbgp, $node2);

	if (!exists($peers1->{$node2})) {
	  print "missing session $node1 --> $node2\n";
	  return 0;
	}
	if (defined($state) &&
	    ($peers1->{$node2}->[F_PEER_STATE] ne $state)) {
	  print "$state\t$peers1->{$node2}->[F_PEER_STATE]\n";
	  return 0;
	}

	if (!exists($peers2->{$node1})) {
	  print "missing session $node2 --> $node1\n";
	  return 0;
	}
	if (defined($state) &&
	    ($peers2->{$node1}->[F_PEER_STATE] ne $state)) {
	  print "$state\t$peers2->{$node1}->[F_PEER_STATE]\n";
	  return 0;
	}
      }
    }
    return 1;
  }

# -----[ cbgp_topo_check_record_route ]------------------------------
# Check that record-routes performed on the given topology correctly
# record the forwarding path. The function can optionaly check that
# the IGP weight and delay of the path are correctly recorded.
#
# Arguments:
#   $cbgp  : CBGP instance
#   $topo  : topology to test
#   $status: expected result (SUCCESS, UNREACH, ...)
#   $delay : true => check that IGP weight and delay are correcly
#            recorded (optional)
#
# Return value:
#   1 success
#   0 failure
# -------------------------------------------------------------------
sub cbgp_topo_check_record_route($$$;$)
  {
    my ($cbgp, $topo, $status, $delay)= @_;
    my $nodes= topo_get_nodes($topo);

    $tests->debug("topo_check_record_route()");

    foreach my $node1 (keys %$nodes) {
      foreach my $node2 (keys %$nodes) {
	($node1 eq $node2) and next;
	my $trace;
	if ($delay) {
	  $trace= cbgp_record_route($cbgp, $node1, $node2,
				    -delay=>1, -weight=>1);
	} else {
	  $trace= cbgp_record_route($cbgp, $node1, $node2);
	}

	return 0
	  if (!defined($trace));

	return 0
	  if (!check_recordroute($trace, -status=>$status));

	# Check that destination matches in case of success
	my @path= @{$trace->[F_TR_PATH]};
	if ($trace->[F_TR_STATUS] eq "SUCCESS") {
	  # Last hop should be destination
	  $tests->debug(join ", ", @path);
	  if ($path[$#path] ne $node2) {
	    $tests->debug("ERROR final node != destination ".
			  "(\"".$path[$#path]."\" != \"$node2\")");
	    return 0;
	  }
	}

	# Optionaly record/check IGP weight and delay
	if (defined($delay) && $delay) {
	  my $delay= 0;
	  my $weight= 0;
	  my $prev_hop= undef;
	  foreach my $hop (@path) {
	    if (defined($prev_hop)) {
	      if (exists($topo->{$prev_hop}{$hop})) {
		$delay+= $topo->{$prev_hop}{$hop}->[0];
		$weight+= $topo->{$prev_hop}{$hop}->[1];
	      } elsif (exists($topo->{$hop}{$prev_hop})) {
		$delay+= $topo->{$hop}{$prev_hop}->[0];
		$weight+= $topo->{$hop}{$prev_hop}->[1];
	      } else {
		return 0;
	      }
	    }
	    $prev_hop= $hop;
	  }
	  $tests->debug("($delay, $weight) vs ($trace->[F_TR_DELAY], $trace->[F_TR_WEIGHT])"); 
	  if ($delay != $trace->[F_TR_DELAY]) {
	    $tests->debug("delay mismatch ($delay vs $trace->[F_TR_DELAY])");
	    return 0;
	  }
	  if ($weight != $trace->[F_TR_WEIGHT]) {
	    return 0;
	  }
	}
      }
    }

    return 1;
  }

# -----[ cbgp_topo_check_static_routes ]-----------------------------
sub cbgp_topo_check_static_routes($$)
  {
    my ($cbgp, $topo)= @_;
    my $nodes= topo_get_nodes($topo);

    foreach my $node1 (keys %$nodes) {
      my $links= cbgp_get_links($cbgp, $node1);

      # Add a static route over each link
      foreach my $link (values %$links) {
	$cbgp->send_cmd("net node $node1 route add --oif=".
			$link->[F_LINK_DST]." ".
			$link->[F_LINK_DST]."/32 ".
			$link->[F_LINK_WEIGHT]);
      }

      # Test static routes (presence)
      my $rt= cbgp_get_rt($cbgp, $node1);
      (scalar(keys %$rt) != scalar(keys %$links)) and return 0;
      foreach my $link (values %$links) {
	my $prefix= $link->[F_LINK_DST]."/32";
	# static route exists
	(!exists($rt->{$prefix})) and return 0;
	# next-hop is link's tail-end
	if ($rt->{$prefix}->[F_RT_IFACE] ne
	    $link->[F_LINK_DST]) {
	  print "next-hop != link's tail-end\n";
	  return 0;
	}
      }
	
      # Test static routes (forward)
      foreach my $link (values %$links) {
	my $trace= cbgp_record_route($cbgp, $node1,
					 $link->[F_LINK_DST]);
	# Destination was reached
	($trace->[F_TR_STATUS] ne "SUCCESS") and return 0;
	# Path is 2 hops long
	(@{$trace->[F_TR_PATH]} != 2) and return 0;
      }
	
    }

    return 1;
  }

# -----[ cbgp_peering ]----------------------------------------------
# Setup a BGP peering between a 'router' and a 'peer' in AS 'asn'.
# Possible options:
#   - virtual
#   - next-hop-self
#   - soft-restart
# -------------------------------------------------------------------
sub cbgp_peering($$$$;@)
  {
    my ($cbgp, $router, $peer, $asn, @options)= @_;

    $cbgp->send_cmd("bgp router $router");
    $cbgp->send_cmd("\tadd peer $asn $peer");
    foreach my $option (@options) {
      $cbgp->send_cmd("\tpeer $peer $option");
    }
    $cbgp->send_cmd("\tpeer $peer up");
    $cbgp->send_cmd("\texit");
  }

# -----[ cbgp_filter ]-----------------------------------------------
# Setup a filter on a BGP peering. The filter specification includes
# a direction (in/out), a predicate and a list of actions.
# -------------------------------------------------------------------
sub cbgp_filter($$$$$$)
  {
    my ($cbgp, $router, $peer, $direction, $predicate, $actions)= @_;

    $cbgp->send_cmd("bgp router $router");
    $cbgp->send_cmd("\tpeer $peer");
    $cbgp->send_cmd("\t\tfilter $direction");
    $cbgp->send_cmd("\t\t\tadd-rule");
    $cbgp->send_cmd("\t\t\t\tmatch '$predicate'");
    $cbgp->send_cmd("\t\t\t\taction \"$actions\"");
    $cbgp->send_cmd("\t\t\t\texit");
    $cbgp->send_cmd("\t\t\texit");
    $cbgp->send_cmd("\t\texit");
    $cbgp->send_cmd("\texit");
  }

# -----[ cbgp_recv_update ]------------------------------------------
sub cbgp_recv_update($$$$$)
  {
    my ($cbgp, $router, $asn, $peer, $msg)= @_;

    $cbgp->send_cmd("bgp router $router peer $peer recv ".
	      "\"BGP4|0|A|$router|$asn|$msg\"");
  }

# -----[ cbgp_recv_withdraw ]----------------------------------------
sub cbgp_recv_withdraw($$$$$)
  {
    my ($cbgp, $router, $asn, $peer, $prefix)= @_;

    $cbgp->send_cmd("bgp router $router peer $peer recv ".
	      "\"BGP4|0|W|$router|$asn|$prefix\"");
  }

# -----[ cbgp_topo_check_links ]-------------------------------------
sub cbgp_topo_check_links($$)
  {
    my ($cbgp, $topo)= @_;
    my %cbgp_topo;
    my %nodes;

    # Retrieve all the links reported by C-BGP through "show links"
    foreach my $node1 (keys %$topo) {
      foreach my $node2 (keys %{$topo->{$node1}}) {
	if (!exists($nodes{$node1})) {
	  $nodes{$node1}= 1;
	  my $links_hash= cbgp_get_links($cbgp, $node1);
	  my @links= values %$links_hash;
	  foreach my $link (@links) {
	    my @attr= ();
	    $attr[F_TOPO_LINK_DELAY]= $link->[F_LINK_DELAY];
	    $attr[F_TOPO_LINK_WEIGHT]= $link->[F_LINK_WEIGHT];
	    $cbgp_topo{$node1}{$link->[F_LINK_DST]}= \@attr;
	  }
	}
	if (!exists($nodes{$node2})) {
	  $nodes{$node2}= 1;
	  my $links_hash= cbgp_get_links($cbgp, $node2);
	  my @links= values %$links_hash;
	  foreach my $link (@links) {
	    my @attr= ();
	    $attr[F_TOPO_LINK_DELAY]= $link->[F_LINK_DELAY];
	    $attr[F_TOPO_LINK_WEIGHT]= $link->[F_LINK_WEIGHT];
	    $cbgp_topo{$node2}{$link->[F_LINK_DST]}= \@attr;
	  }
	}
      }
    }

    # Compare these links to the initial topology
    return (topo_included($topo, \%cbgp_topo) &&
	    topo_included(\%cbgp_topo, $topo));
  }

# -----[ cbgp_check_bgp_route ]--------------------------------------
sub cbgp_check_bgp_route($$$)
  {
    my ($cbgp, $node, $prefix)= @_;

    my $rib= cbgp_get_rib($cbgp, $node);
    if (!exists($rib->{$prefix})) {
      return 0;
    } else {
      return 1;
    }
  }

#####################################################################
#
# VALIDATION TESTS
#
#####################################################################

# -----[ cbgp_valid_version ]----------------------------------------
# Check that the version returned by C-BGP is correctly formated.
#
# This test is mainly used to check that C-BGP can be executed and
# we are able to talk with it. We also derive the default report
# filename prefix from the returned C-BGP version (it can be
# overriden with the --report-prefix option).
# -------------------------------------------------------------------
sub cbgp_valid_version($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("show version");
    $cbgp->send_cmd("print \"DONE\\n\"");
    while (!((my $result= $cbgp->expect(1)) =~ m/^DONE/)) {
      if ($result =~ m/^([a-z]+)\s+version:\s+([0-9]+)\.([0-9]+)\.([0-9]+)(\-([^ ]+))?(\s+.+)*$/) {
	my $component= $1;
	my $version= "$2.$3.$4";
	if (defined($5)) {
	  $version.= " ($5)";
	}
	if ($component eq "cbgp") {
	  (defined($validation->{'cbgp_version'})) and
	    return TEST_FAILURE;
	  if (($2 < CBGP_MAJOR_MIN()) ||
	      (($2 == CBGP_MAJOR_MIN()) && ($3 < CBGP_MINOR_MIN))) {
	    show_warning("this validation script was designed for version ".
			 CBGP_MAJOR_MIN().".".CBGP_MINOR_MIN().".x");
	  }
	  $validation->{'cbgp_version'}= $version;
	  if ($report_prefix eq DEFAULT_REPORT_PREFIX) {
	    $report_prefix= "cbgp-$2.$3.$4";
	    (defined($5)) and $report_prefix.= $5;
	    $report_prefix.= '-valid';
	  }
	} elsif ($component eq "libgds") {
	  (defined($validation->{'libgds_version'})) and
	    return TEST_FAILURE;
	  $validation->{'libgds_version'}= $version;
	} else {
	  show_warning("unknown component \"$component\"");
	  return TEST_FAILURE;
	}
      } else {
	show_warning("syntax error");
	return TEST_FAILURE;
      }
    }
    return (defined($validation->{'cbgp_version'}) &&
	    defined($validation->{'libgds_version'}))?TEST_SUCCESS:TEST_FAILURE;
  }

# -----[ load_tests ]------------------------------------------------
# Tests are Perl scripts located in the ./tests subdirectory.
# Each script provides a single test.
# -------------------------------------------------------------------
sub load_tests($) {
  my ($dir)= @_;

  opendir(TESTS, $dir) or die "Could not read \"$dir\" directory";
 LOOP_DIR: while (my $dir_entry= readdir(TESTS)) {
    # Only keep regular files
    (! -f "$dir/$dir_entry") and next LOOP_DIR;
    # Only keep files with .pl extension
    (!($dir_entry=~ m/\.pl$/)) and next LOOP_DIR;

    # Process file
    my $result= do "$dir/$dir_entry";

    # Skip if 'do' could not read the file (undef was returned)
    if (!defined($result)) {
      ($@) and show_warning("could not parse file \"$dir/$dir_entry\" ($@)");
      ($!) and show_warning("could not read file  \"$dir/$dir_entry\" ($!)");
      next LOOP_DIR;
    }

    # Check that result is an array reference of size 2
    if ((ref($result) ne 'ARRAY') || ((scalar(@$result) < 2) && (scalar(@$result) > 3))) {
      show_warning("registration failed for \"$dir/$dir_entry\"");
      next LOOP_DIR;
    }

    # Register test
    $tests->debug("Loading \"$dir/$dir_entry\" ($result->[0])");
    $tests->register("$dir/$dir_entry", @$result);
  }
  closedir(TESTS);
}

# -----[ run_tests ]-------------------------------------------------
#
# -------------------------------------------------------------------
sub run_tests() {
  if ($tests->run() > 0) {
    show_error("".$tests->{'num-failures'}." test(s) failed.");
    return -1;
  } else {
    show_info("all tests passed.");
    return 0;
  }
}

# -----[ build_reports ]---------------------------------------------
#
# -------------------------------------------------------------------
sub build_reports() {
  return
    if (!exists($opts{report}));

  # Parse files for self-documentation
  foreach my $test (@{$tests->{'list'}}) {
    $test->{TEST_FIELD_DOC}=
      CBGPValid::BaseReport::doc_from_script($test->{TEST_FIELD_FILE});
  }

  if ($opts{report} eq "html") {
    CBGPValid::HTMLReport::report_write("$report_prefix",
					$validation,
					$tests);
  } elsif ($opts{report} eq "xml") {
    CBGPValid::XMLReport::report_write("$report_prefix",
				       $validation,
				       $tests);
    CBGPValid::BaseReport::save_resources("$report_prefix.resources");
  } else {
    show_error("Unknown reporting type \"$opts{report}\"");
  }
}


#####################################################################
#
# MAIN
#
#####################################################################

# -------------------------------------------------------------------
# Note: the "show version" test has a special treatment as it is used
# to check that we can talk with a cbgp instance and to retrieve the
# instance's version.
# -------------------------------------------------------------------
show_info("Loading tests...");
$tests->register("---", "show version", "cbgp_valid_version");
load_tests("tests");
my $return_value= run_tests();
build_reports();
show_info("done.");
exit($return_value);
