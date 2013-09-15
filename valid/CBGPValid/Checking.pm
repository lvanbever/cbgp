# ===================================================================
# CBGPValid:Checking.pm
#
# Functions to check constraints on information (links, routes, BGP
# routes) retrieved from a C-BGP instance.
#
# author Bruno Quoitin (bruno.quoitin@umons.ac.be)
# $Id: Checking.pm,v 1.5 2009-06-25 14:36:27 bqu Exp $
# ===================================================================

package CBGPValid::Checking;

require Exporter;

@ISA= qw(Exporter);
@EXPORT= qw(
	    cbgp_check_link
	    check_has_link
	    cbgp_check_link

	    cbgp_check_route
	    check_has_route
	    check_route

	    check_has_iface
	    check_iface

	    cbgp_check_peer
	    check_has_peer
	    check_peer

	    cbgp_rib_check
	    check_has_bgp_route
	    check_bgp_route

	    check_traceroute
	    check_recordroute
	    check_ping
	    check_link_info
	    check_node_info

	    aspath_equals
	    comm_equals
	    clusterlist_equals
	   );

use strict;
use CBGPValid::Functions;

my $tests;
1;

use constant VAL_FIELD      => 0;
use constant VAL_CONSTRAINT => 1;


# -----[ set_tests ]-------------------------------------------------
sub set_tests($) {
  ($tests)= @_;
}

# -----[ cbgp_check_link ]-------------------------------------------
sub cbgp_check_link($$$%) {
  my ($cbgp, $src, $dest, %args)= @_;
  $tests->debug("cbgp_check_link($src, $dest)");
  my $links= cbgp_get_links($cbgp, $src);
  if (keys(%$links) == 0) {
    $tests->debug("no link from \"$src\"");
    return 0;
  }
  return check_has_link($links, $dest, %args);
}

# -----[ check_has_link ]--------------------------------------------
sub check_has_link($$%) {
  my ($links, $dest, %args)= @_;
  if (!exists($links->{$dest})) {
    $tests->debug("no link towards \"$dest\"");
    return 0;
  }
  return check_link($links->{$dest}, %args);

}

# -----[ check_link ]------------------------------------------------
sub check_link($%) {
  my ($link, %args)= @_;
  my %fields= (
	       -delay  => [F_LINK_DELAY],
	       -type   => [F_LINK_TYPE],
	       -weight => [F_LINK_WEIGHT],
	      );
  $tests->debug("check_link($link->[F_LINK_DST], ".
		__constraints_to_str(\%fields, \%args).")");
  return __constraints_check($link, \%fields, \%args);
}

# -----[ cbgp_check_route ]------------------------------------------
sub cbgp_check_route($$$%) {
  my ($cbgp, $src, $dest, %args)= @_;
  my $rt= cbgp_get_rt($cbgp, $src, $dest);
  return check_has_route($rt, $dest, %args);
}

# -----[ check_has_route ]-------------------------------------------
sub check_has_route($$%) {
  my ($rt, $dest, %args)= @_;
  $dest= canonic_prefix($dest);
  if (!exists($rt->{$dest})) {
    $tests->debug("no route towards \"$dest\"");
    return 0;
  }
  return check_route($rt->{$dest}, %args);
}

# -----[ check_route ]-----------------------------------------------
sub check_route($%) {
  my ($route, %args)= @_;
  my %fields= (
	       -iface   => [F_RT_IFACE],
	       -metric  => [F_RT_METRIC],
	       -nexthop => [F_RT_NEXTHOP],
	       -proto   => [F_RT_PROTO],
	       -ecmp    => [F_RT_ECMP,
			    \&rt_ecmp_equals,
			    \&rt_ecmp_to_str],
	      );
  $tests->debug("check_route($route->[F_RT_DEST], ".
		__constraints_to_str(\%fields, \%args).")");
  return __constraints_check($route, \%fields, \%args);
}

# -----[ check_has_iface ]-------------------------------------------
sub check_has_iface($$%) {
  my ($ifaces, $id, %args)= @_;
  if (!exists($ifaces->{$id})) {
    $tests->debug("no interface with id \"$id\"");
    return 0;
  }
  return check_iface($ifaces->{$id}, %args);
}

# -----[ check_iface ]-----------------------------------------------
sub check_iface($%) {
  my ($iface, %args)= @_;
  my %fields= (
	       -type => [F_IFACE_TYPE],
	      );
  $tests->debug("check_iface($iface->[F_IFACE_ID], ".
		__constraints_to_str(\%fields, \%args).")");
  return __constraints_check($iface, \%fields, \%args);
}

# -----[ cbgp_check_peer ]-------------------------------------------
sub cbgp_check_peer($$$%) {
  my ($cbgp, $router, $peer, %args)= @_;
  $tests->debug("cbgp_check_peer($router, $peer)");
  my $peers= cbgp_get_peers($cbgp, $router);
  return check_has_peer($peers, $peer, %args);
}

# -----[ check_has_peer ]--------------------------------------------
sub check_has_peer($$%) {
  my ($peers, $id, %args)= @_;
  if (!exists($peers->{$id})) {
    $tests->debug("no peer with id \"$id\"");
    return 0;
  }
  return check_peer($peers->{$id}, %args);
}

# -----[ check_peer ]------------------------------------------------
sub check_peer($%) {
  my ($peer, %args)= @_;
  my %fields= (
	       -state => [F_PEER_STATE],
	       );
  $tests->debug("check_peer($peer->[F_PEER_IP], ".
		__constraints_to_str(\%fields, \%args).")");
  return __constraints_check($peer, \%fields, \%args);
}

# -----[ cbgp_rib_check ]--------------------------------------------
sub cbgp_rib_check($$%) {
  my ($rib, $prefix, %args)= @_;
  return check_has_bgp_route($rib, $prefix, %args);
}

# -----[ check_has_bgp_route ]---------------------------------------
sub check_has_bgp_route($$%) {
  my ($rib, $prefix, %args)= @_;
  $prefix= canonic_prefix($prefix);
  if (!exists($rib->{$prefix})) {
    $tests->debug("no route towards \"$prefix\"");
    return 0;
  }
  return check_bgp_route($rib->{$prefix}, %args);
}

# -----[ check_bgp_route ]-------------------------------------------
sub check_bgp_route($%) {
  my ($route, %args)= @_;
  my %fields= (
	       -clusterlist => [F_RIB_CLUSTERLIST,
				\&clusterlist_equals, \&clusterlist_to_str],
	       -community   => [F_RIB_COMMUNITY, \&comm_equals, \&comm_to_str],
	       -med         => [F_RIB_MED],
	       -nexthop     => [F_RIB_NEXTHOP],
	       -originator  => [F_RIB_ORIGINATOR],
	       -pref        => [F_RIB_PREF],
	       -path        => [F_RIB_PATH, \&aspath_equals, \&aspath_to_str],
	      );
  $tests->debug("check_bgp_route($route->[F_RIB_PREFIX], ".
		__constraints_to_str(\%fields, \%args).")");
  return __constraints_check($route, \%fields, \%args);
}

# -----[ check_traceroute ]------------------------------------------
sub check_traceroute($%) {
  my ($trace, %args)= @_;
  my %fields= (
	       -nhops  => [F_TRACEROUTE_NHOPS],
	       -hops   => [F_TRACEROUTE_HOPS,
			   \&traceroute_hops_equals, \&traceroute_hops_to_str],
	       -status => [F_TRACEROUTE_STATUS]
	      );
  $tests->debug("check_traceroute(".
	       __constraints_to_str(\%fields, \%args).")");
  return __constraints_check($trace, \%fields, \%args);
}

# -----[ check_recordroute ]-----------------------------------------
sub check_recordroute($%) {
  my ($trace, %args)= @_;
  my %fields= (
	       -status   => [F_TR_STATUS],
	       -path     => [F_TR_PATH,
			     \&recordroute_hops_equals,
			     \&recordroute_hops_to_str],
	       -capacity => [F_TR_CAPACITY],
	       -delay    => [F_TR_DELAY],
	       -weight   => [F_TR_WEIGHT],
	      );
  $tests->debug("check_recordroute(".
	       __constraints_to_str(\%fields, \%args).")");
  return __constraints_check($trace, \%fields, \%args);
}

# -----[ check_ping ]-----------------------------------------
sub check_ping($%) {
  my ($ping, %args)= @_;
  my %fields= (
	       -status => [F_PING_STATUS],
	       -addr   => [F_PING_ADDR],
	      );
  $tests->debug("check ping(".__constraints_to_str(\%fields, \%args).")");
  return __constraints_check($ping, \%fields, \%args);
}

# -----[ check_link_info ]-------------------------------------------
sub check_link_info($%) {
  my ($info, %args)= @_;
  my %fields= (
	       -load => 'load',
	      );
  $tests->debug("check_link_info(".__info_constraints_to_str(\%args).")");
  return __check_info_constraints($info, \%fields, \%args);
}

# -----[ check_node_info ]-------------------------------------------
sub check_node_info($%) {
  my ($info, %args)= @_;
  my %fields= (
	       -name => 'name',
	      );
  $tests->debug("check_node_info(".__info_constraints_to_str(\%args).")");
  return __check_info_constraints($info, \%fields, \%args);
}

# -----[ __info_constraints_to_str ]---------------------------------
sub __info_constraints_to_str($) {
  my ($args)= @_;
  my $str= '';
  for my $ct (keys %$args) {
    ($str ne '') and $str.= ' ,';
    if (defined($args->{$ct})) {
      $str.= "$ct=$args->{$ct}";
    } else {
      $str.= "$ct=undef";
    }
  }
  return $str;
}

# -----[ __check_info_constraints ]----------------------------------
sub __check_info_constraints($$) {
  my ($info, $fields, $args)= @_;

  for my $ct (keys %$args) {
    die "unsupported link info constraints \"$ct\""
      if (!exists($fields->{$ct}));
    my $ct_field= $fields->{$ct};
    my $ct_value= $args->{$ct};
    if (!defined($ct_value)) {
      # Field must not exist or must have an undefined value
      if (exists($info->{$ct_field})) {
	$tests->debug("field \"$ct_field\" should be undefined");
	return 0;
      }
    } else {
      # Field must exist and have given value
      if (!exists($info->{$ct_field})) {
	$tests->debug("field \"$ct_field\" is not defined");
	return 0;
      } elsif ($info->{$ct_field} ne $ct_value) {
	$tests->debug("field value ($info->{$ct_field}) does not match ".
		      "constraint ($ct_value)");
	return 0;
      }
    }
  }
  return 1;
}


#####################################################################
#
# SPECIAL TYPES CHECKING
#
#####################################################################

# -----[ aspath_to_str ]---------------------------------------------
sub aspath_to_str($) {
  my ($aspath)= @_;
  if (defined($aspath)) {
    return "[".(join ",", @$aspath)."]";
  }
  return "undef";
}

# -----[ comm_to_str ]-----------------------------------------------
sub comm_to_str($) {
  my ($comm)= @_;
  if (defined($comm)) {
    return "[".(join ",", @$comm)."]";
  }
  return "undef";
}

# -----[ clusterlist_to_str ]----------------------------------------
sub clusterlist_to_str() {
  my ($cl)= @_;
  if (defined($cl)) {
    return "[".(join ",", @$cl)."]";
  }
  return "undef";
}

# -----[ aspath_equals ]---------------------------------------------
# Compare two AS-Paths.
#
# Return 1 if both AS-Paths are equal
#        0 if they differ
#
# Note: the function only supports AS-Paths composed of AS_SEQUENCEs
# -------------------------------------------------------------------
sub aspath_equals($$) {
  my ($path1, $path2)= @_;

  (!defined($path1)) and $path1= [];
  (!defined($path2)) and $path2= [];

  $tests->debug("aspath_equals([".(join " ", @$path1)."], ".
		"[".(join " ", @$path2)."])");

  (@$path1 != @$path2) and return 0;
  for (my $index= 0; $index < @$path1; $index++) {
    ($path1->[$index] != $path2->[$index]) and return 0;
  }
  return 1;
}

# -----[ comm_equals ]-----------------------------------------------
# Compare two sets of communities.
#
# Return 1 if both sets contain the same communities (ordering does
#          not matter!)
#        0 if sets differ
# -------------------------------------------------------------------
sub comm_equals($$) {
  my ($comm1, $comm2)= @_;

  if (!defined($comm1)) {
    (defined($comm2) && @$comm2) and return 0;
    (!defined($comm2) || !@$comm2) and return 1;
  }
  if (!defined($comm2)) {
    (defined($comm1) && @$comm1) and return 0;
    (!defined($comm1) || !@$comm1) and return 1;
  }

  if (@$comm1 != @$comm2) {
    $tests->debug("community length mismatch");
    return 0;
  }
  my %comm1;
  my %comm2;
  foreach my $comm (@$comm1) {
    $comm1{$comm}= 1;
  }
  foreach my $comm (@$comm2) {
    $comm2{$comm}= 1;
  }
  foreach my $comm (keys %comm1) {
    if (!exists($comm2{$comm})) {
      $tests->debug("$comm not in comm2");
      return 0;
    }
  }
  foreach my $comm (keys %comm2) {
    if (!exists($comm1{$comm})) {
      $tests->debug("$comm not in comm1");
      return 0;
    }
  }
  return 1;
}

# -----[ clusterlist_equals ]----------------------------------------
# Compare two cluster-id-lists.
#
# Return 1 if both lists are equal (ordering matters!)
#        0 if they differ
# -------------------------------------------------------------------
sub clusterlist_equals($$) {
  my ($cl1, $cl2)= @_;

  if (!defined($cl1)) {
    (defined($cl2) && @$cl2) and return 0;
    (!defined($cl2) || !@$cl2) and return 1;
  }
  if (!defined($cl2)) {
    (defined($cl1) && @$cl1) and return 0;
    (!defined($cl1) || !@$cl1) and return 1;
  }

  if (@$cl1 != @$cl2) {
    $tests->debug("cluster-id-list length mismatch");
    return 0;
  }

  for (my $index= 0; $index < @$cl1; $index++) {
    ($cl1->[$index] ne $cl2->[$index]) and return 0;
  }
  return 1;
}

# -----[ traceroute_hops_to_str ]------------------------------------
sub traceroute_hops_to_str($$) {
  my ($hop, $type)= @_;
  if ($type == VAL_FIELD) {
    my $str= "";
    foreach my $h (@$hop) {
      $str= $str."(".$h->[0].','.$h->[1].','.$h->[2].")";
    }
    return $str;
  } elsif ($type == VAL_CONSTRAINT) {
    my $str= "";
    foreach my $h (@$hop) {
      $str= $str."($h->[0],$h->[1],$h->[2])";
    }
    return $str;
  }
  return undef;
}

# -----[ traceroute_hops_equals ]------------------------------------
sub traceroute_hops_equals($$) {
  my ($hops, $hops_ct)= @_;
  for my $hop (@$hops_ct) {
    my $index= $hop->[F_HOP_INDEX];
    if ($index >= scalar(@$hops)) {
      $tests->debug("hop $index does not exist (hop-count:".
		    scalar(@$hops).")");
      return 0;
    }
    my $chop= $hops->[$index];
    if ($chop->[F_HOP_ADDRESS] ne $hop->[F_HOP_ADDRESS]) {
      $tests->debug("hop $index address ($chop->[F_HOP_ADDRESS]) ".
		    "does not match ($hop->[F_HOP_ADDRESS])");
      return 0;
    }
    if ($chop->[F_HOP_STATUS] ne $hop->[F_HOP_STATUS]) {
      $tests->debug("hop $index status ($chop->[F_HOP_STATUS]) ".
		    "does not match ($hop->[F_HOP_STATUS])");
      return 0;
    }
  }
  return 1;
}

# -----[ recordroute_hops_to_str ]-----------------------------------
sub recordroute_hops_to_str($$);
sub recordroute_hops_to_str($$) {
  my ($hop, $type)= @_;
  if ($type == VAL_FIELD) {
    my $str= "";
    for my $h (@$hop) {
      if (ref($h) eq 'ARRAY') {
	$str= $str."{".recordroute_hops_to_str($h, $type)."}";
      } else {
	$str= $str."($h)";
      }
    }
    return $str;
  } else {
    my $str= "";
    for my $h (@$hop) {
      if (ref($h->[1]) eq 'ARRAY') {
	$str= $str."{".recordroute_hops_to_str($h->[1], $type)."}";
      } else {
	$str= $str."($h->[0],$h->[1])";
      }
    }
    return $str;
  }
  return undef;
}

sub recordroute_hops_equals($$);
# -----[ recordroute_hops_equals ]-----------------------------------
sub recordroute_hops_equals($$) {
  my ($path, $path_ct)= @_;
  for my $hop (@$path_ct) {
    my $index= $hop->[0];
    if ($index >= scalar(@$path)) {
      $tests->debug("hop $index does not exist (hop-count:".
		    scalar(@$path).")");
      return 0;
    }
    my $chop= $path->[$index];
    if (ref($hop->[1]) eq 'ARRAY') {
      (!recordroute_hops_equals($chop, $hop->[1])) and
	return 0;
    } else {
      if ($chop ne $hop->[1]) {
	$tests->debug("hop $index address ($chop) ".
		      "does not match ($hop->[1])");
	return 0;
      }
    }
  }
  return 1;
}

sub rt_ecmp_equals($$) {
  my ($value, $constraint)= @_;
  return check_route($value, %$constraint);
}

sub rt_ecmp_to_str($$$$) {
  my ($ecmp, $type, $fields, $args)= @_;
  if ($type == VAL_FIELD) {
    my $str= '';
    foreach my $f (@$ecmp) {
      ($str ne '') and $str.= ',';
      $str.= $f;
    }
    return '('.$str.')';
  } else {
    return "(".__constraints_to_str($fields, $ecmp).")";
  }
}

#####################################################################
#
# CONSTRAINTS CHECKING API
#
#####################################################################

# -----[ __constraints_val_to_str ]----------------------------------
sub __constraints_val_to_str($$$$$) {
  my ($fields, $args, $field, $value, $type)= @_;
  my $func= undef;
  if (exists($fields->{$field})) {
    my $data= $fields->{$field};
    $func= $data->[2];
    if (defined($func)) {
      return &$func($value,$type,$fields,$args);
    }
  }
  if (!defined($value)) {
    return "undef";
  } else {
    return $value;
  }
}

# -----[ __constraints_to_str ]--------------------------------------
sub __constraints_to_str($$) {
  my ($fields, $args)= @_;
  my $str= '';
  foreach my $a (keys %$args) {
    ($str ne '') and $str.= ', ';
    my $value= $args->{$a};
    $str.= "$a=".__constraints_val_to_str($fields, $args, $a, $value,
					  VAL_CONSTRAINT);
  }
  return $str;
}

# -----[ __constraints_check ]---------------------------------------
sub __constraints_check($$$) {
  my ($target, $constraints, $args)= @_;
  foreach my $a (keys %$args) {
    die "unsupported constraint \"$a\"" if (!exists($constraints->{$a}));
    my $data= $constraints->{$a};
    my $f= $data->[0];
    my $test;

    my $value= $target->[$f];
    my $cons= $args->{$a};

    if (scalar(@$data) > 1) {
      my $func= $data->[1];
      $test= !&$func($value, $cons);
    } else {
      if (!defined($value) && !defined($cons)) {
	$test= 0; # success
      } elsif (!defined($value) || !defined($cons)) {
	$test= 1; # failure
      } else {
	$test= ($value ne $cons);
      }
    }

    if ($test) {
      my $field_val= __constraints_val_to_str($constraints, $args, $a, $value,
					     VAL_FIELD);
      my $cons_val= __constraints_val_to_str($constraints, $args, $a, $cons,
					    VAL_CONSTRAINT);
      $tests->debug("$a \"$field_val\" does not match ".
		    "constraint \"$cons_val\"");
      return 0;
    }
  }
  return 1;
}

# -----[ canonic_prefix ]--------------------------------------------
#
# -------------------------------------------------------------------
sub canonic_prefix($) {
  my ($prefix)= @_;
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

