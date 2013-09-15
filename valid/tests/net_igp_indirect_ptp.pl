use strict;

return ["net igp indirect (ptp)",
	"cbgp_valid_net_igp_indirect_ptp"];

# -----[ cbgp_valid_net_igp_indirect ]-------------------------------
# Check how local interfaces on a distant node (at one hop) are
# handled in the IGP model. The distant node is connected through a
# ptp link.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#   - N1 (192.168.0.0/24)
#   - N2 (192.168.1.0/30)
#   - N3 (192.168.1.4/30)
#   - cost(R1->R2): 1
#   - cost(R1->N1): 10
#   - cost(R1->R3): 100
#   - cost(R4->R1): 10000
#
# Topology:
#
#  R4 --N3--*   *------ R2
#     .6  .5 \ /
#             R1
#         .1 / \
#  N1 ------*   *--N2-- R3
#               .1    .2
#
# Scenario:
#   * Compute IGP routes
#   * Check that R4 has a route to R2 with cost equal to
#     cost(R1->R2)+cost(R4->R1)
#   * Check that R4 has a route to N1 with cost equal to
#     cost(R1->N1)+cost(R4->R1)
#   * Check that R4 has a route to N2 with cost equal to
#     cost(R1->R3)+cost(R4->R1)
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_indirect_ptp($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net add subnet 192.168.0.0/24 stub");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net add link-ptp 1.0.0.1 192.168.1.5/30 1.0.0.4 192.168.1.6/30");
  $cbgp->send_cmd("net add link-ptp 1.0.0.1 192.168.1.1/30 1.0.0.3 192.168.1.2/30");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24");

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.1 192.168.0.1/24 igp-weight 10");
  $cbgp->send_cmd("net node 1.0.0.1 iface 192.168.1.1 igp-weight 100");
  $cbgp->send_cmd("net node 1.0.0.3 iface 192.168.1.2 igp-weight 1000");
  $cbgp->send_cmd("net node 1.0.0.1 iface 192.168.1.5 igp-weight 10000");
  $cbgp->send_cmd("net node 1.0.0.4 iface 192.168.1.6 igp-weight 10000");
  $cbgp->send_cmd("net domain 1 compute");

  my $rt= cbgp_get_rt($cbgp, '1.0.0.4', '*');
  return TEST_FAILURE
    if (!check_has_route($rt, '1.0.0.2/32', -metric=>10001));
  return TEST_FAILURE
    if (!check_has_route($rt, '192.168.0.0/24', -metric=>10010));
  return TEST_FAILURE
    if (!check_has_route($rt, '192.168.1.0/30', -metric=>10100));

  return TEST_SUCCESS;
}
