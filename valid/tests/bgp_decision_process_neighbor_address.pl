use strict;

return ["bgp decision process neighbor address",
	"cbgp_valid_bgp_dp_neighbor_address"];

# -----[ cbgp_valid_bgp_dp_neighbor_address ]------------------------
# Test ability to break ties based on the neighbor's IP address.
# Note: this can only occur in the case were there are at least
# two BGP sessions between two routers.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS2)
#   - R3 (2.0.0.1, AS2) virtual peer of R1
#
# Topology:
#
#               *--(192.168.0.0/31)--*
#              /                      \
#   R3 ----- R1                        R2
#              \                      /
#               *--(192.168.0.2/31)--*
#
# Scenario:
#   * Advertise 255/8 from 2.0.0.1 to 1.0.0.1
#   * Check that 1.0.0.2 has two routes, one with next-hop
#     192.168.0.0 and the other one with next-hop 192.168.0.2
#   * Check that best route is through 192.168.0.0 (lowest neighbor
#     address)
# -------------------------------------------------------------------
sub cbgp_valid_bgp_dp_neighbor_address($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add node 2.0.0.1");
  $cbgp->send_cmd("net node 2.0.0.1 domain 1");
  $cbgp->send_cmd("net add subnet 192.168.0.0/31 transit");
  $cbgp->send_cmd("net add subnet 192.168.0.2/31 transit");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.0/31");
  $cbgp->send_cmd("net link 1.0.0.1 192.168.0.0/31 igp-weight 1");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.2/31");
  $cbgp->send_cmd("net link 1.0.0.1 192.168.0.2/31 igp-weight 1");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.1/31");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.0.1/31 igp-weight 1");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.3/31");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.0.3/31 igp-weight 1");
  $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1");
  $cbgp->send_cmd("net link 1.0.0.1 2.0.0.1 igp-weight 1");
  $cbgp->send_cmd("net domain 1 compute");

  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("\tadd peer 1 192.168.0.1");
  $cbgp->send_cmd("\tpeer 192.168.0.1 next-hop 192.168.0.0");
  $cbgp->send_cmd("\tpeer 192.168.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 192.168.0.3");
  $cbgp->send_cmd("\tpeer 192.168.0.3 next-hop 192.168.0.2");
  $cbgp->send_cmd("\tpeer 192.168.0.3 up");
  $cbgp->send_cmd("\tadd peer 2 2.0.0.1");
  $cbgp->send_cmd("\tpeer 2.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 2.0.0.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tadd peer 1 192.168.0.0");
  $cbgp->send_cmd("\tpeer 192.168.0.0 up");
  $cbgp->send_cmd("\tadd peer 1 192.168.0.2");
  $cbgp->send_cmd("\tpeer 192.168.0.2 up");
  $cbgp->send_cmd("\texit");

  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2|IGP|2.0.0.1|0|0");

  $cbgp->send_cmd("sim run");
  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.2");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"192.168.0.0"));
  return TEST_SUCCESS;
}

