use strict;

return ["net igp ecmp", "cbgp_valid_net_igp_ecmp"];

# -----[ cbgp_valid_net_igp_ecmp ]-----------------------------------
# Test the ECMP capability of the IGP model on a simple topology.
#
# Setup:
#   - R1 (0.0.0.1)
#   - R2 (0.0.0.2)
#   - R3 (0.0.0.3)
#   - R4 (0.0.0.4)
#   - all links have an IGP weight equal to 1
#
# Topology:
#
#     *-- R2 --*
#    /          \
#   R1          R4
#    \          /
#     *-- R3 --*
#
# Scenario:
#   * this setup should provide 2 different paths from R1 to R4
#     (R1, R2, R4)
#     (R1, R3, R4)
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_ecmp($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add node 0.0.0.1");
  $cbgp->send_cmd("net add node 0.0.0.2");
  $cbgp->send_cmd("net add node 0.0.0.3");
  $cbgp->send_cmd("net add node 0.0.0.4");
  $cbgp->send_cmd("net add link 0.0.0.1 0.0.0.2");
  $cbgp->send_cmd("net add link 0.0.0.1 0.0.0.3");
  $cbgp->send_cmd("net add link 0.0.0.2 0.0.0.4");
  $cbgp->send_cmd("net add link 0.0.0.3 0.0.0.4");

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 0.0.0.1 domain 1");
  $cbgp->send_cmd("net node 0.0.0.2 domain 1");
  $cbgp->send_cmd("net node 0.0.0.3 domain 1");
  $cbgp->send_cmd("net node 0.0.0.4 domain 1");
  $cbgp->send_cmd("net link 0.0.0.1 0.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.1 0.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.2 0.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.3 0.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");

  # Note: the first next-hop is through the lowest interface address
  #       this is deterministic and due to how c-bgp orders routing
  #       table entries

  my $rt= cbgp_get_rt($cbgp, '0.0.0.1', '0.0.0.4');
  return TEST_FAILURE
    if (!check_has_route($rt, '0.0.0.4/32',
			 -nexthop=>'0.0.0.0',
			 -iface=>'0.0.0.2',
			 -ecmp=>{
				 -nexthop=>'0.0.0.0',
				 -iface=>'0.0.0.3'
				 }
			));

  return TEST_SUCCESS;
}
