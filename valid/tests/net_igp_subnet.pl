use strict;

return ["net igp subnet", "cbgp_valid_net_igp_subnet"];

# -----[ cbgp_valid_net_igp_subnet ]---------------------------------
# Check that the IGP routing protocol model handles subnets
# correctly.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - N1 (192.168.0/24, transit)
#   - N2 (192.168.1/24, stub)
#   - N3 (192.168.2/24, stub)
#
# Topology:
#        10        10        10
#   R1 ------{N1}------ R2 ------{N3}
#    |                  |
#  1 |                  | 1
#    |                  |
#   R3 ------{N2}-------*
#         1
#
# Scenario:
#   * create topology and compute IGP routes
#   * check that R1 joins R2 through R1->N1 (next-hop = R2.N1)
#   * check that R1 joins R3 through R1->R3
#   * check that R1 joins N1 throuth R1->N1
#   * check that R1 joins N2 through R1->R3
#   * check that R1 joins N3 through R1->N1 (next-hop = R2.N1)
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_subnet($) {
  my ($cbgp)= @_;
  my $rt;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
  $cbgp->send_cmd("net add subnet 192.168.1/24 stub");
  $cbgp->send_cmd("net add subnet 192.168.2/24 stub");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight 1");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.1 igp-weight 1");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24");
  $cbgp->send_cmd("net link 1.0.0.1 192.168.0.1/24 igp-weight 10");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.2/24");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.0.2/24 igp-weight 10");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.1.2/24");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.1.2/24 igp-weight 1");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.2.2/24");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.2.2/24 igp-weight 10");
  $cbgp->send_cmd("net add link 1.0.0.3 192.168.1.3/24");
  $cbgp->send_cmd("net link 1.0.0.3 192.168.1.3/24 igp-weight 1");
  $cbgp->send_cmd("net domain 1 compute");

  $rt= cbgp_get_rt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_route($rt, "1.0.0.2/32",
			 -nexthop=>"192.168.0.2"));
  return TEST_FAILURE
    if (!check_has_route($rt, "1.0.0.3/32"));
  return TEST_FAILURE
    if (!check_has_route($rt, "192.168.0.0/24"));
  return TEST_FAILURE
    if (!check_has_route($rt, "192.168.1.0/24"));
  return TEST_FAILURE
    if (!check_has_route($rt, "192.168.2.0/24",
			 -nexthop=>"192.168.0.2"));
  return TEST_SUCCESS;
}

