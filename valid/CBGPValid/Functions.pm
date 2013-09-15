# ===================================================================
# CBGPValid:Functions.pm
#
# Functions to retrieve information (nodes, links, routes, BGP
# routes) from a C-BGP instance.
#
# author Bruno Quoitin (bruno.quoitin@umons.ac.be)
# $Id: Functions.pm,v 1.7 2009-06-25 14:36:27 bqu Exp $
# ===================================================================

package CBGPValid::Functions;

require Exporter;
use CBGPValid::Topologies;

@ISA= qw(Exporter);
@EXPORT= qw(F_IFACE_ID
	    F_IFACE_TYPE
	    F_IFACE_LOCAL
	    F_IFACE_DEST

	    F_LINK_TYPE
	    F_LINK_IFACE
	    F_LINK_DST
	    F_LINK_WEIGHT
	    F_LINK_DELAY
	    F_LINK_STATE
	    F_LINK_LOAD

	    F_RT_DEST
	    F_RT_NEXTHOP
	    F_RT_IFACE
	    F_RT_METRIC
	    F_RT_PROTO
	    F_RT_ECMP

	    F_PEER_IP
	    F_PEER_AS
	    F_PEER_STATE
	    F_PEER_ROUTERID
	    F_PEER_OPTIONS

	    F_RIB_STATUS
	    F_RIB_PEER_IP
	    F_RIB_PEER_AS
	    F_RIB_PREFIX
	    F_RIB_NEXTHOP
	    F_RIB_PREF
	    F_RIB_MED
	    F_RIB_PATH
	    F_RIB_ORIGIN
	    F_RIB_COMMUNITY
	    F_RIB_ECOMMUNITY
	    F_RIB_ORIGINATOR
	    F_RIB_CLUSTERLIST

	    F_TR_SRC
	    F_TR_DST
	    F_TR_STATUS
	    F_TR_NUMHOPS
	    F_TR_PATH
	    F_TR_WEIGHT
	    F_TR_DELAY
	    F_TR_CAPACITY

	    F_HOP_INDEX
	    F_HOP_ADDRESS
	    F_HOP_STATUS
	    F_HOP_IFACE

	    F_PING_ADDR
	    F_PING_STATUS

	    F_TRACEROUTE_STATUS
	    F_TRACEROUTE_NHOPS
	    F_TRACEROUTE_HOPS

	    C_IFACE_LOOPBACK
	    C_IFACE_RTR
	    C_IFACE_PTP
	    C_IFACE_PTMP
	    C_IFACE_VIRTUAL

	    C_RT_PROTO_BGP
	    C_RT_PROTO_IGP
	    C_RT_PROTO_STATIC

	    C_PEER_ACTIVE
	    C_PEER_ESTABLISHED
	    C_PEER_IDLE
	    C_PEER_OPENWAIT

	    C_TRACEROUTE_STATUS_REPLY
	    C_TRACEROUTE_STATUS_NO_REPLY
	    C_TRACEROUTE_STATUS_ERROR

	    C_PING_STATUS_REPLY
	    C_PING_STATUS_NO_REPLY
	    C_PING_STATUS_ICMP_TTL
	    C_PING_STATUS_ICMP_NET
	    C_PING_STATUS_ICMP_HOST
	    C_PING_STATUS_ICMP_PROTO
	    C_PING_STATUS_NET
	    C_PING_STATUS_HOST

	    cbgp_checkpoint

	    cbgp_get_links
	    cbgp_get_ifaces
	    cbgp_get_rt
	    cbgp_get_peers
	    cbgp_get_rib
	    cbgp_get_rib_in
	    cbgp_get_rib_mrt
	    cbgp_get_rib_custom
	    cbgp_get_routers

	    cbgp_node_info
	    cbgp_link_info
	    cbgp_bgp_router_info

	    cbgp_record_route
	    cbgp_bgp_record_route
	    cbgp_ping
	    cbgp_traceroute

	    bgp_route_parse_cisco
	    bgp_route_parse_mrt

	    cbgp_bgp_topology
	   );

use strict;
use CBGPValid::UI;

#####################################################################
#
# CONSTANTS
#
#####################################################################

# -----[ Fields : Interface ]-----
use constant F_IFACE_ID    => 0;
use constant F_IFACE_TYPE  => 1;
use constant F_IFACE_DEST  => 2;

# -----[ Fields : IP link ]-----
use constant F_LINK_TYPE   => 0;
use constant F_LINK_IFACE  => 1;
use constant F_LINK_DST    => 2;
use constant F_LINK_WEIGHT => 3;
use constant F_LINK_DELAY  => 4;
use constant F_LINK_STATE  => 5;
use constant F_LINK_LOAD   => 6;

# -----[ Fields : IP route ]-----
use constant F_RT_DEST    => 0;
use constant F_RT_NEXTHOP => 1;
use constant F_RT_IFACE   => 2;
use constant F_RT_METRIC  => 3;
use constant F_RT_PROTO   => 4;
use constant F_RT_ECMP    => 5;

# -----[ Fields : BGP peer ]-----
use constant F_PEER_IP => 0;
use constant F_PEER_AS => 1;
use constant F_PEER_STATE => 2;
use constant F_PEER_ROUTERID => 3;
use constant F_PEER_OPTIONS => 4;

# -----[ Fields : BGP route ]-----
use constant F_RIB_STATUS      => 0;
use constant F_RIB_PEER_IP     => 1;
use constant F_RIB_PEER_AS     => 2;
use constant F_RIB_PREFIX      => 3;
use constant F_RIB_NEXTHOP     => 4;
use constant F_RIB_PREF        => 5;
use constant F_RIB_MED         => 6;
use constant F_RIB_PATH        => 7;
use constant F_RIB_ORIGIN      => 8;
use constant F_RIB_COMMUNITY   => 9;
use constant F_RIB_ECOMMUNITY  => 10;
use constant F_RIB_ORIGINATOR  => 11;
use constant F_RIB_CLUSTERLIST => 12;

# -----[ Fields : traceroute ]-----
use constant F_TR_SRC      => 0;
use constant F_TR_DST      => 1;
use constant F_TR_STATUS   => 2;
use constant F_TR_NUMHOPS  => 3;
use constant F_TR_PATH     => 4;
use constant F_TR_WEIGHT   => 5;
use constant F_TR_DELAY    => 6;
use constant F_TR_CAPACITY => 7;

use constant F_HOP_INDEX   => 0;
use constant F_HOP_ADDRESS => 1;
use constant F_HOP_STATUS  => 2;
use constant F_HOP_IFACE   => 3;

# -----[ Fields : ping ]-----
use constant F_PING_ADDR   => 0;
use constant F_PING_STATUS => 1;

# -----[ Fields : traceroute ]-----
use constant F_TRACEROUTE_STATUS => 0;
use constant F_TRACEROUTE_NHOPS  => 1;
use constant F_TRACEROUTE_HOPS   => 2;

# -----[ Constants : network interface types ]-----
use constant C_IFACE_LOOPBACK => 'lo';
use constant C_IFACE_RTR      => 'rtr';
use constant C_IFACE_PTP      => 'ptp';
use constant C_IFACE_PTMP     => 'ptmp';
use constant C_IFACE_VIRTUAL  => 'virtual';

# -----[ Constants : IP route protocol ]-----
use constant C_RT_PROTO_BGP    => "BGP";
use constant C_RT_PROTO_IGP    => "IGP";
use constant C_RT_PROTO_STATIC => "STATIC";

# -----[ Constants : Peer state ]-----
use constant C_PEER_ACTIVE      => "ACTIVE";
use constant C_PEER_ESTABLISHED => "ESTABLISHED";
use constant C_PEER_IDLE        => "IDLE";
use constant C_PEER_OPENWAIT    => "OPENWAIT";

# -----[ Constants : traceroute status ]-----
use constant C_TRACEROUTE_STATUS_REPLY    => 'reply';
use constant C_TRACEROUTE_STATUS_NO_REPLY => 'no reply';
use constant C_TRACEROUTE_STATUS_ERROR    => 'error';

# -----[ Constants : ping status ]-----
use constant C_PING_STATUS_REPLY      => 'reply';
use constant C_PING_STATUS_NO_REPLY   => 'no reply';
use constant C_PING_STATUS_ICMP_TTL   => 'icmp error (time-exceeded)';
use constant C_PING_STATUS_ICMP_NET   => 'icmp error (network-unreachable)';
use constant C_PING_STATUS_ICMP_HOST  => 'icmp error (host-unreachable)';
use constant C_PING_STATUS_ICMP_PROTO => 'icmp error (prococol-unreachable)';
use constant C_PING_STATUS_NET        => 'network unreachable';
use constant C_PING_STATUS_HOST       => 'host unreachable';


#####################################################################
#
# FUNCTIONS
#
#####################################################################

# -----[ check_address ]---------------------------------------------
# Parse an IP address.
#
# Returns the address.
# Returns undef in case the input string is not an IP address.
# -------------------------------------------------------------------
sub check_address($)
{
  my ($address)= @_;

  if (!($address =~ m/^([0-9.]+)$/)) {
    show_error("invalid IP address: $address");
    return undef;
  }
  return $address;
}

# -----[ check_prefix ]----------------------------------------------
# Parse an IP prefix and returns an array with the network and mask
# parts.
#
# Returns an array with
#   [0] address
#   [1] mask
# Returns undef in case the input string is not an IP prefix.
# -------------------------------------------------------------------
sub check_prefix($)
{
  my ($prefix)= @_;

  if (!($prefix =~ m/^([0-9.]+)\/([0-9]+)$/)) {
    show_error("invalid IP prefix: $prefix");
    return undef;
  }
  my ($addr, $mask)= ($1, $2);
  return ($addr, $mask);
}

# -----[ cbgp_parse_link ]-------------------------------------------
sub cbgp_parse_link($)
{
  my ($string)= @_;

  my @fields= split /\s+/, $string;
  if (scalar(@fields) < 6) {
    show_error("not enough fields (parse link): \"$string\"");
    return undef;
  }

  my @link= ();

  my $findex= 0;
  $link[F_LINK_TYPE]= $fields[$findex++];

  # Interface identifier
  $link[F_LINK_IFACE]= $fields[$findex++];

  # Destination identifier (depends on interface type)
  if ($link[F_LINK_TYPE] eq C_IFACE_LOOPBACK) {
    $link[F_LINK_DST]= "loopback";
    $findex++;
  } elsif ($link[F_LINK_TYPE] eq C_IFACE_RTR) {
    my $address= check_address($fields[$findex]);
    if (!defined($address)) {
      show_error("invalid rtr-dst (show links): ".$fields[$findex]);
      return undef;
    }
    $link[F_LINK_DST]= $address;
    $findex++;
  } elsif ($link[F_LINK_TYPE] eq C_IFACE_PTMP) {
    my @prefix= check_prefix($fields[$findex]);
    if (!scalar(@prefix)) {
      show_error("invalid ptmp-dst (show links): ".$fields[$findex]);
      return undef;
    }
    $link[F_LINK_DST]= $prefix[0]."/".$prefix[1];
    $findex++;
  } else {
    show_error("unsupported interface-type (show links): $fields[0]");
    return undef;
  }

  # Link delay
  if (!($fields[$findex] =~ m/^[0-9]+$/)) {
    show_error("invalid delay (show links): $fields[$findex]");
    return undef;
  }
  $link[F_LINK_DELAY]= $fields[$findex++];

  # Link weight
  if (!($fields[$findex] =~ m/^[0-9]+$/)) {
    show_error("invalid weight (show links): $fields[$findex]");
    return undef;
  }
  $link[F_LINK_WEIGHT]= $fields[$findex++];

  # Link state (UP/DOWN)
  if (!($fields[$findex] =~ m/^UP|DOWN$/)) {
    show_error("invalid state (show links): $fields[$findex++]");
    return undef;
  }
  $link[F_LINK_STATE]= $fields[$findex++];

  return \@link;
}

# -----[ cbgp_checkpoint ]-------------------------------------------
# Wait until C-BGP has finished previous treatment...
#
# A second argument can be provided if the function must return all
# the text that precedes the checkpoint. The second argument must be
# a reference to an array. Each line of text received will be pushed
# in this array.
# -------------------------------------------------------------------
sub cbgp_checkpoint($;$) {
  my ($cbgp, $data)= @_;
  $cbgp->send_cmd("print \"CHECKPOINT\\n\"");
  while ((my $result= $cbgp->expect(1)) ne "CHECKPOINT") {
    (defined($data)) and
      push @$data, ($result);
    select(undef,undef,undef,0.01); # sleep 1/100th of a second
    #sleep(1);
  }
}

# -----[ cbgp_get_links ]--------------------------------------------
# Get the list of links of one node. Parse the output of the "net
# node show links" command.
#
# Returns a hash with
#   (keys)   = (destinations)
#   (values) = (links)
#
# Warning: this function will not work if there are multiple links
#          towards the same subnet (LAN) on the same node. In
#          practice, this should be a rare situation (according to
#          a CISCO source).
# -------------------------------------------------------------------
sub cbgp_get_links($$)
{
  my ($cbgp, $node)= @_;
  my %links= ();

  $cbgp->send_cmd("net node $node show links");
  $cbgp->send_cmd("print \"done\\n\"");
  while ((my $result= $cbgp->expect(1)) ne "done") {
    my $link= cbgp_parse_link($result);
    if (!defined($link)) {
      show_error("incorrect format (show links): \"$result\"");
      exit(-1);
    }
    if (exists($links{$link->[F_LINK_DST]})) {
      show_error("duplicate link destination: ".$link->[F_LINK_DST]."");
      exit(-1);
    }
    $links{$link->[F_LINK_DST]}= $link;
  }
  return \%links;
}

# -----[ cbgp_parse_iface ]------------------------------------------
sub cbgp_parse_iface($)
{
  my ($string)= @_;

  my @fields= split /\s+/, $string;
  if (scalar(@fields) < 2) {
    show_error("not enough fields (parse iface): \"$string\"");
    return undef;
  }
  my @iface= ();

  # Interface type
  if (($fields[0] eq C_IFACE_LOOPBACK) ||
      ($fields[0] eq C_IFACE_RTR) ||
      ($fields[0] eq C_IFACE_PTP) ||
      ($fields[0] eq C_IFACE_PTMP) ||
      ($fields[0] eq C_IFACE_VIRTUAL)) {
    $iface[F_IFACE_TYPE]= $fields[0];
  } else {
    show_error("invalid interface-type (parse iface): \"$fields[0]\"");
    return undef;
  }

  # Interface ID
  my @prefix= check_prefix($fields[1]);
  if (!scalar(@prefix)) {
    show_error("invalid interface-ID (parse iface): \"$fields[1]\"");
    return undef;
  }
  $iface[F_IFACE_ID]= $prefix[0]."/".$prefix[1];

  $iface[F_IFACE_DEST]= undef;
  return \@iface;
}

# -----[ cbgp_get_ifaces ]-------------------------------------------
# Get the list of interfaces of one node. Parse the output of the
# "net node show ifaces" command.
#
# Returns a hash with
#   (keys)   = (iface-id)
#   (values) = (iface)
# -------------------------------------------------------------------
sub cbgp_get_ifaces($$)
  {
    my ($cbgp, $node)= @_;
    my %ifaces;

    $cbgp->send_cmd("net node $node show ifaces");
    $cbgp->send_cmd("print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      my $iface= cbgp_parse_iface($result);
      if (!defined($iface)) {
	show_error("invalid format(show ifaces): \"$result\"");
	exit(-1);
      }
      if (exists($ifaces{$iface->[F_IFACE_ID]})) {
	show_error("duplicate interface (show ifaces): $iface->[F_IFACE_ID]");
	exit(-1);
      }
      $ifaces{$iface->[F_IFACE_ID]}= $iface;
    }
    return \%ifaces;
  }

# -----[ cbgp_get_rt ]-----------------------------------------------
# Get the list of routes of one node. Parse the output of the "net
# node show rt" command.
#
# Optional argument:
#   destination (IP address | IP prefix | *)
#
# Returns a hash with
#  (keys)   = (dst-prefix)
#  (values) = (route)
# -------------------------------------------------------------------
sub cbgp_get_rt($$;$)
  {
    my ($cbgp, $node, $destination)= @_;
    my %routes;
    my $last_pfx= undef;

    if (!defined($destination)) {
      $destination= "*";
    }

    $cbgp->send_cmd("net node $node show rt $destination");
    $cbgp->send_cmd("print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      my @fields= split /\s+/, $result;
      if (scalar(@fields) < 5) {
	show_error("incorrect format (show rt): \"$result\"");
	exit(-1);
      }

      if ($fields[0] eq '') {
	if (!defined($last_pfx)) {
	  show_error("no destination (show rt): $fields[0]");
	  exit(-1);
	}
	$fields[0]= $last_pfx;
      } elsif (!($fields[0] =~ m/^[0-9.\/]+$/)) {
	show_error("invalid destination (show rt): $fields[0]");
	exit(-1);
      }
      $last_pfx= $fields[0];

      if (!($fields[1] =~ m/^[0-9.]+$/)) {
	show_error("invalid next-hop (show rt): $fields[1]");
	exit(-1);
      }
      if (($fields[2] ne '---') && (!($fields[2] =~ m/^[0-9.\/]+$/))) {
	show_error("invalid interface (show rt): $fields[2]");
	exit(-1);
      }
      if (!($fields[3] =~ m/^[0-9]+$/)) {
	show_error("invalid metric (show rt): $fields[3]");
	exit(-1);
      }
      if (!($fields[4] =~ m/^STATIC|IGP|BGP$/)) {
	show_error("invalid type (show rt): $fields[4]");
	exit(-1);
      }

      my @route= ();
      $route[F_RT_DEST]= $fields[0];
      $route[F_RT_NEXTHOP]= $fields[1];
      $route[F_RT_IFACE]= $fields[2];
      $route[F_RT_METRIC]= $fields[3];
      $route[F_RT_PROTO]= $fields[4];
      if (!exists($routes{$fields[F_RT_DEST]})) {
	$routes{$fields[F_RT_DEST]}= \@route;
      } else {
	my $prev_route= $routes{$fields[F_RT_DEST]};
	while (defined($prev_route->[F_RT_ECMP])) {
	  $prev_route= $prev_route->[F_RT_ECMP];
	}
	$prev_route->[F_RT_ECMP]= \@route;
      }
    }
    return \%routes;
  }

# -----[ cbgp_get_peers ]-------------------------------------------
sub cbgp_get_peers($$)
  {
    my ($cbgp, $node)= @_;
    my %peers;

    $cbgp->send_cmd("bgp router $node show peers");
    $cbgp->send_cmd("print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      if ($result =~ m/^([0-9.]+)\s+AS([0-9]+)\s+([A-Z]+)\s+([0-9.]+)(\s+.+)?$/) {
	my @peer;
	$peer[F_PEER_IP]= $1;
	$peer[F_PEER_AS]= $2;
	$peer[F_PEER_STATE]= $3;
	$peer[F_PEER_ROUTERID]= $4;
	$peer[F_PEER_OPTIONS]= $5;
	$peers{$1}= \@peer;
      } else {
	show_error("incorrect format (show peers): \"$result\"");
	exit(-1);
      }
    }
    return \%peers;
  }

# -----[ bgp_route_parse_cisco ]-------------------------------------
# Parse a CISCO "show ip bgp" route.
#
# Return:
#   - in case of success: an anonymous array containing the
#                         route's  attributes.
#   - in case of failure: undef
# -------------------------------------------------------------------
sub bgp_route_parse_cisco($)
  {
    my ($string)= @_;

    if ($string =~ m/^([i*>]+)\s+([0-9.\/]+)\s+([0-9.]+)\s+([0-9]+)\s+([0-9]+)\s+([^\t]*)\s+([ie\?])$/) {
      my @route;
      $route[F_RIB_STATUS]= $1;
      $route[F_RIB_PREFIX]= $2;
      $route[F_RIB_NEXTHOP]= $3;
      $route[F_RIB_PREF]= $4;
      $route[F_RIB_MED]= $5;
      my @path= ();
      ($6 ne "null") and @path= split /\s+/, $6;
      $route[F_RIB_PATH]= \@path;
      #$route[F_RIB_]= $7;
      return \@route;
    }

    return undef;
  }

# -----[ bgp_route_parse_mrt ]---------------------------------------
# Parse an MRT route.
#
# Return:
#   - in case of success: an anonymous array containing the
#                         route's  attributes.
#   - in case of failure: undef
# -------------------------------------------------------------------
sub bgp_route_parse_mrt($)
  {
    my ($string)= @_;

    if ($string =~
	m/^(.?)\|(LOCAL|[0-9.]+)\|(LOCAL|[0-9]+)\|([0-9.\/]+)\|([0-9 null]*)\|(IGP|EGP|INCOMPLETE)\|([0-9.]+)\|([0-9]+)\|([0-9]*)\|.+$/) {
      my @fields= split /\|/, $string;
      my @route;
      $route[F_RIB_STATUS]= $fields[0];
      $route[F_RIB_PEER_IP]= $fields[1];
      $route[F_RIB_PEER_AS]= $fields[2];
      $route[F_RIB_PREFIX]= $fields[3];
      $route[F_RIB_NEXTHOP]= $fields[6];
      my @path= ();
      ($fields[4] ne "null") and @path= split /\s/, $fields[4];
      $route[F_RIB_PATH]= \@path;
      $route[F_RIB_PREF]= $fields[7];
      $route[F_RIB_MED]= $fields[8];
      if (@fields > 9) {
	my @communities= split /\s/, $fields[9];
	$route[F_RIB_COMMUNITY]= \@communities;
      } else {
	$route[F_RIB_COMMUNITY]= undef;
      }
      if (@fields > 10) {
	my @communities= split /\s/, $fields[10];
	$route[F_RIB_ECOMMUNITY]= \@communities;
      } else {
	$route[F_RIB_ECOMMUNITY]= undef;
      }
      if (@fields > 11) {
	$route[F_RIB_ORIGINATOR]= $fields[11];
      } else {
	$route[F_RIB_ORIGINATOR]= undef;
      }
      if (@fields > 12) {
	my @clusterlist= split /\s/, $fields[12];
	$route[F_RIB_CLUSTERLIST]= \@clusterlist;
      } else {
	$route[F_RIB_CLUSTERLIST]= undef;
      }
      return \@route;
    }

    return undef;
  }

# -----[ cbgp_get_routers ]------------------------------------------
# Get all the routers defined in C-BGP.
#
# Arguments:
#   $cbgp  : C-BGP instance
#   $prefix: optional prefix (can be undefined)
# -------------------------------------------------------------------
sub cbgp_get_routers($;$)
  {
    my ($cbgp, $prefix)= @_;
    my %routers;

    (!defined($prefix)) and $prefix= '*';

    $cbgp->send_cmd("bgp show routers $prefix");
    $cbgp->send_cmd("print \"CHECKPOINT\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "CHECKPOINT") {
      if ($result =~ m/^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$/) {
	if (defined($routers{$result})) {
	  show_error("duplicate router (bgp show routers): $result");
	}
	$routers{$result}= 1;
      } else {
	show_error("invalid format (bgp show routers): $result");
      }
    }
    return \%routers;
  }

# -----[ cbgp_get_rib ]---------------------------------------------
sub cbgp_get_rib($$;$)
  {
    my ($cbgp, $node, $destination)= @_;
    my %rib;

    if (!defined($destination)) {
      $destination= "*";
    }

    $cbgp->send_cmd("bgp router $node show rib $destination");
    $cbgp->send_cmd("print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      my $route= bgp_route_parse_cisco($result);
      if (!defined($route)) {
	show_error("incorrect format (show rib): \"$result\"");
	exit(-1);
      }
      $rib{$route->[F_RIB_PREFIX]}= $route;
    }
    return \%rib;
  }

# -----[ cbgp_get_rib_in ]------------------------------------------
sub cbgp_get_rib_in($$;$$)
  {
    my ($cbgp, $node, $peer, $destination)= @_;
    my %rib;

    if (!defined($peer)) {
      $peer= '*';
    }

    if (!defined($destination)) {
      $destination= '*';
    }

    $cbgp->send_cmd("bgp router $node show adj-rib in $peer $destination");
    $cbgp->send_cmd("print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      my $route= bgp_route_parse_cisco($result);
      if (!defined($route)) {
	show_error("incorrect format (show rib): \"$result\"");
	exit(-1);
      }
      $rib{$route->[F_RIB_PREFIX]}= $route;
    }
    return \%rib;
  }

# -----[ cbgp_get_rib_mrt ]-----------------------------------------
sub cbgp_get_rib_mrt($$;$)
  {
    my ($cbgp, $node, $destination)= @_;
    my %rib;

    if (!defined($destination)) {
      $destination= "*";
    }

    $cbgp->send_cmd("bgp options show-mode mrt");
    $cbgp->send_cmd("bgp router $node show rib $destination");
    $cbgp->send_cmd("print \"done\\n\"");
    $cbgp->send_cmd("bgp options show-mode cisco");
    while ((my $result= $cbgp->expect(1)) ne "done") {

      my $route= bgp_route_parse_mrt($result);
      if (!defined($route)) {
	show_error("incorrect format (show rib mrt): \"$result\"");
	exit(-1);
      }
      $rib{$route->[F_RIB_PREFIX]}= $route;
    }
    return \%rib;
  }

# -----[ cbgp_get_rib_custom ]--------------------------------------
sub cbgp_get_rib_custom($$;$)
  {
    my ($cbgp, $node, $destination)= @_;
    my %rib;

    if (!defined($destination)) {
      $destination= "*";
    }

    $cbgp->send_cmd("bgp options show-mode \"?|%i|%a|%p|%P|%o|%n|%l|%m|%c|%e|%O|%C|\"");
    $cbgp->send_cmd("bgp router $node show rib $destination");
    $cbgp->send_cmd("print \"done\\n\"");
    $cbgp->send_cmd("bgp options show-mode cisco");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      if ($result =~
	  m/^(.?)\|(LOCAL|[0-9.]+)\|(LOCAL|[0-9]+)\|([0-9.\/]+)\|([0-9 null]+)\|(IGP|EGP|INCOMPLETE)\|([0-9.]+)\|([0-9]+)\|([0-9]*)\|.+$/) {
	my @fields= split /\|/, $result;

	my @route;
	$route[F_RIB_STATUS]= 0; #$fields[0];
	$route[F_RIB_PEER_IP]= $fields[1];
	$route[F_RIB_PEER_AS]= $fields[2];
	$route[F_RIB_PREFIX]= $fields[3];
	$route[F_RIB_ORIGIN]= $fields[5];
	$route[F_RIB_NEXTHOP]= $fields[6];
	my @path= ();
	($fields[4] ne "null") and @path= split /\s/, $fields[4];
	$route[F_RIB_PATH]= \@path;
	$route[F_RIB_PREF]= $fields[7];
	$route[F_RIB_MED]= $fields[8];
	if (@fields > 9) {
	  my @communities= split /\s/, $fields[9];
	  $route[F_RIB_COMMUNITY]= \@communities;
	} else {
	  $route[F_RIB_COMMUNITY]= undef;
	}
	if (@fields > 10) {
	  my @communities= split /\s/, $fields[10];
	  $route[F_RIB_ECOMMUNITY]= \@communities;
	} else {
	  $route[F_RIB_ECOMMUNITY]= undef;
	}
	if (@fields > 11) {
	  $route[F_RIB_ORIGINATOR]= $fields[11];
	} else {
	  $route[F_RIB_ORIGINATOR]= undef;
	}
	if (@fields > 12) {
	  my @clusterlist= split /\s/, $fields[12];
	  $route[F_RIB_CLUSTERLIST]= \@clusterlist;
	} else {
	  $route[F_RIB_CLUSTERLIST]= undef;
	}
	$rib{$4}= \@route;
      } else {
	show_error("incorrect format (show rib mrt): \"$result\"");
	exit(-1);
      }
    }
    return \%rib;
  }


#####################################################################
#
# Infos
#
#####################################################################

# -----[ cbgp_node_info ]--------------------------------------------
sub cbgp_node_info($$)
  {
    my ($cbgp, $node)= @_;
    my %info;

    $cbgp->send_cmd("net node $node show info");
    $cbgp->send_cmd("print \"done\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "done") {
      if (!($result =~ m/^([a-z]+)\s*\:\s*(.*)$/)) {
	show_error("incorrect format (net node show info): \"$result\"");
	exit(-1);
      }
      $info{$1}= $2;
    }
    return \%info;
  }

# -----[ cbgp_link_info ]--------------------------------------------
sub cbgp_link_info($$$) {
  my ($cbgp, $src, $dst)= @_;
  my %info;

  $cbgp->send_cmd("net link $src $dst show info");
  $cbgp->send_cmd("print \"done\\n\"");
  while ((my $result= $cbgp->expect(1)) ne "done") {
    if (!($result =~ m/^([a-z\-0-9]+)\s*\:\s*(.*)$/)) {
      show_error("incorrect format (net link show info): \"$result\"");
      exit(-1);
    }
    $info{$1}= $2;
  }
  return \%info;
}

# -----[ cbgp_bgp_router_info ]--------------------------------------
sub cbgp_bgp_router_info($$) {
  my ($cbgp, $router)= @_;
  my %info;

  $cbgp->send_cmd("bgp router $router show info");
  $cbgp->send_cmd("print \"done\\n\"");
  while ((my $result= $cbgp->expect(1)) ne "done") {
    if (!($result =~ m/^([a-z]+)\s*\:\s*(.*)$/)) {
      show_error("incorrect format (bgp router show info): \"$result\"");
      exit(-1);
    }
    $info{$1}= $2;
  }
  return \%info;
}


#####################################################################
#
# Ping, traceroute and record-route
#
#####################################################################

# -----[ _net_record_route_options ]---------------------------------
sub _net_record_route_options(%) {
  my (%args)= @_;
  my $options= "";
  (exists($args{-capacity})) and $options.= "--capacity ";
  (exists($args{-checkloop})) and $options.= "--check-loop ";
  (exists($args{-delay})) and $options.= "--delay ";
  (exists($args{-weight})) and $options.= "--weight ";
  (exists($args{-load})) and $options.= "--load=".$args{-load}." ";
  (exists($args{-ecmp})) and $options.= "--ecmp ";
  (exists($args{-tunnel})) and $options.= "--tunnel ";
  return $options;
}

sub _net_record_route_parse_sub_trace($$);
# -----[ _net_record_route_parse_sub_trace ]-------------------------
sub _net_record_route_parse_sub_trace($$) {
  my ($fields, $field_index)= @_;
  my @trace= ();
  my $finished= 0;
  while (!$finished) {
    my $hop= $fields->[$field_index];
    if ($hop =~ m/^(.+)\]$/) {
      $hop= $1;
      $finished= 1;
    } elsif ($hop =~ m/^\[(.+)$/) {
      $fields->[$field_index]= $1;
      $hop= _net_record_route_parse_sub_trace($fields, $field_index);
      $field_index+= scalar(@$hop);
    } else {
      $field_index++;
    }
    push @trace, ($hop);
  };
  return \@trace;
}

# -----[ _net_record_route_parser ]----------------------------------
sub _net_record_route_parser($;%) {
  my ($result, %args)= @_;
  my @trace;

  my @fields= split /\s/, $result;
  if (scalar(@fields) < 4) {
    show_error("not enough fields (record-route): \"$result\"");
    return undef;
  }
  if (!($fields[0] =~ m/^[0-9.]+$/)) {
    show_error("invalid source (record-route): \"$result\"");
    return undef;
  }
  if (!($fields[1] =~ m/^[0-9.\/]+$/)) {
    show_error("invalid destination (record-route): \"$result\"");
    return undef;
  }
  if (!($fields[2] =~ m/^[A-Z_]+$/)) {
    show_error("invalid status (record-route): \"$result\"");
    return undef;
  }
  if (!($fields[3] =~ m/^[0-9]+$/)) {
    show_error("invalid number of hops (record-route): \"$result\"");
    return undef;
  }
  $trace[F_TR_SRC]= $fields[0];
  $trace[F_TR_DST]= $fields[1];
  $trace[F_TR_STATUS]= $fields[2];
  $trace[F_TR_NUMHOPS]= $fields[3];

  # Parse paths
  my $field_index= 4;
  my @path= ();
  my $path_ref= \@path;
  $trace[F_TR_PATH]= $path_ref;
  my @stack= ();
  for (my $index= 0; $index < $trace[F_TR_NUMHOPS]; $index++) {
    if ($field_index >= scalar(@fields)) {
      show_error("invalid hop[$index] (record-route)");
      return undef;
    }
    my $hop= $fields[$field_index];
    if (!($hop =~ m/^(\[?)([0-9.]+)$/)) {
      show_error("invalid hop[$index] (record-route): \"$hop\"");
      return undef;
    }
    $hop= $2;
    if ($1 ne '') {
      $fields[$field_index]= $2;
      $hop= _net_record_route_parse_sub_trace(\@fields, $field_index);
      $field_index+= scalar(@$hop);
    } else {
      $field_index++;
    }
    push @$path_ref, ($hop);
  }

  # Parse options
  my %options;
  while ($field_index < scalar(@fields)) {
    if (!($fields[$field_index] =~ m/^([a-z]+)\:(.+)$/)) {
      show_error("invalid option (record-route): \"$fields[$field_index]\"");
      return undef;
    }
    $options{$1}= $2;
    $field_index++;
  }

  if (exists($args{-capacity})) {
    if (!exists($options{capacity})) {
      show_error("missing option \"capacity\" (record-route): \"$result\"");
      return undef;
    }
    $trace[F_TR_CAPACITY]= $options{capacity};
  }
  if (exists($args{-delay})) {
    if (!exists($options{delay})) {
      show_error("missing option \"delay\" (record-route): \"$result\"");
      return undef;
    }
    $trace[F_TR_DELAY]= $options{delay};
  }
  if (exists($args{-weight})) {
    if (!exists($options{weight})) {
      show_error("missing option \"weight\" (record-route): \"$result\"");
      return undef;
    }
    $trace[F_TR_WEIGHT]= $options{weight};
  }

  return \@trace;
}

# -----[ cbgp_record_route ]-----------------------------------------
# Record the route from a source node to a destination node or
# prefix. The function can optionaly request that the route's IGP
# weight and delay be recorded.
#
# Arguments:
#   $cbgp : CBGP instance
#   $src  : IP address of source node
#   $dst  : IP address or IP prefix
#   %args:
#     -capacity=>1  : record max-capacity
#     -checkloop=>1 : check for forwarding loop
#     -delay=>1     : record delay
#     -load=>LOAD   : load path with LOAD
#     -weight=>1    : record IGP weight
#     -ecmp=>1      : record all equal-cost paths
#
# Note: if the -ecmp option is set, the function will return a
#       reference to an anonymous array that contains all the traces.
# -------------------------------------------------------------------
sub cbgp_record_route($$$;%) {
  my ($cbgp, $src, $dst, %args)= @_;
  my $options= _net_record_route_options(%args);
  my $result;

  $cbgp->send_cmd("net node $src record-route $options $dst");
  if (exists($args{-ecmp})) {
    $cbgp->send_cmd("print \"CHECKPOINT\\n\"");
    my @traces;
    while (($result= $cbgp->expect(1)) ne "CHECKPOINT") {
      push @traces, _net_record_route_parser($result, %args);
    }
    return \@traces;
  } else {
    $result= $cbgp->expect(1);
    return _net_record_route_parser($result, %args);
  }
}

# -----[ cbgp_bgp_record_route ]-------------------------------------
sub cbgp_bgp_record_route($$$)
  {
    my ($cbgp, $src, $dst)= @_;

    $cbgp->send_cmd("bgp router $src record-route $dst\n");
    my $result= $cbgp->expect(1);
    if ($result =~ m/^([0-9.]+)\s+([0-9.\/]+)\s+([A-Z_]+)\s+([0-9\s]*)$/) {
      my @fields= split /\s+/, $result;
      my @trace;
      $trace[F_TR_SRC]= shift @fields;
      $trace[F_TR_DST]= shift @fields;
      $trace[F_TR_STATUS]= shift @fields;
      $trace[F_TR_PATH]= \@fields;
      return \@trace;
    } else {
      show_error("incorrect format (bgp record-route): \"$result\"");
      exit(-1);
    }
  }

# -----[ cbgp_ping ]-------------------------------------------------
# Ping a destination from a source node.
#
# Arguments:
#   $cbgp : CBGP instance
#   $src  : IP address of source node
#   $dst  : IP address of destination node
#   %args:
#     -ttl=>TTL  : set time-to-live to TTL
# -------------------------------------------------------------------
sub cbgp_ping($$$;%)
  {
    my ($cbgp, $src, $dst, %args)= @_;
    my $options= "";
    (exists($args{-ttl})) and $options.= "--ttl=".$args{-ttl};

    $cbgp->send_cmd("net node $src ping $options $dst");
    my $result= $cbgp->expect(1);
    if ($result =~ m/^ping\:\s+([a-z\-\(\) ]+)$/) {
      my @ping;
      $ping[F_PING_ADDR]= undef;
      $ping[F_PING_STATUS]= $1;
      return \@ping;
    } elsif ($result =~ m/^ping:\s+(.+)\s+from\s+([0-9.]+)$/) {
      my @ping;
      $ping[F_PING_ADDR]= $2;
      $ping[F_PING_STATUS]= $1;
      return \@ping;
    } else {
      show_error("incorrect format (net ping): \"$result\"");
      exit(-1);
    }
  }

# -----[ cbgp_traceroute ]-------------------------------------------
# Traceroute a destination from a source node.
#
# Arguments:
#   $cbgp : CBGP instance
#   $src  : IP address of source node
#   $dst  : IP address of destination node
# -------------------------------------------------------------------
sub cbgp_traceroute($$$)
  {
    my ($cbgp, $src, $dst)= @_;
    my $hop= 1;
    my $status= C_TRACEROUTE_STATUS_ERROR;
    my @traceroute;
    my @hops;

    $cbgp->send_cmd("net node $src traceroute $dst");
    $cbgp->send_cmd("print \"CHECKPOINT\\n\"");
    while ((my $result= $cbgp->expect(1)) ne "CHECKPOINT") {
      if ($result =~ m/^\s*([0-9]+)\s+([0-9.]+|\*)\s+\(([0-9.]+|\*)\)\s+(.+)$/) {
	if ($hop != $1) {
	  show_error("incorrect hop number \"$result\"");
	  exit(-1);
	}
	my @hop_info= ();
	$hop_info[F_HOP_INDEX]= $1;  # hop index
	$hop_info[F_HOP_ADDRESS]= $2;  # node address
	$hop_info[F_HOP_IFACE]= $3;  # interface address
 	$hop_info[F_HOP_STATUS]= $4;  # status
	push @hops, (\@hop_info);
	if ($4 eq C_PING_STATUS_REPLY) {
	  $status= C_TRACEROUTE_STATUS_REPLY;
	} elsif ($4 eq C_PING_STATUS_NO_REPLY) {
	  $status= C_TRACEROUTE_STATUS_NO_REPLY;
	}
      } else {
	show_error("incorrect format (net traceroute): \"$result\"");
	exit(-1);
      }
      $hop++;
    }

    $traceroute[F_TRACEROUTE_STATUS]= $status;
    $traceroute[F_TRACEROUTE_NHOPS]= $hop-1;
    $traceroute[F_TRACEROUTE_HOPS]= \@hops;
    return \@traceroute;
  }

# -----[ cbgp_bgp_topology ]-----------------------------------------
# Obtain the AS-level topology from C-BGP
# -------------------------------------------------------------------
sub cbgp_bgp_topology($;$) {
  my ($cbgp, $addr_sch)= @_;
  $cbgp->send_cmd("bgp topology dump");
  my @data;
  cbgp_checkpoint($cbgp, \@data);
  return topo_from_subramanian_array(\@data, $addr_sch);
}

