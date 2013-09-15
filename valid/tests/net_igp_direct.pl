use strict;

return ["net igp direct",
	"cbgp_valid_net_igp_direct"];

# -----[ cbgp_valid_net_igp_direct ]---------------------------------
# Check how local interfaces are handled in the IGP model. These
# routes are typically stored in the "direct" routing table on
# genuine routers. It may be the case in a future version of C-BGP.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - N1 (192.168.0.0/24)
#   - N2 (192.168.1.0/30)
#   - cost(R1->R2): 1
#   - cost(R1->N1): 10
#   - cost(R1->R3): 100
#
# Topology:
#
#             *------ R2
#            /
#  N1 ----- R1
#        .1  \
#             *--N2-- R3
#            .1     .2
#
# Scenario:
#   * Compute IGP routes
#   * Check that R1 has a route to R2 with cost equal to cost(R1->R2)
#   * Check that R1 has a route to N1 with cost equal to cost(R1->N1)
#   * Check that R1 has a route to N2 with cost equal to cost(R1->R3)
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_direct($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add subnet 192.168.0.0/24 stub");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net add link-ptp 1.0.0.1 192.168.1.1/30 1.0.0.3 192.168.1.2/30");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24");

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.1 192.168.0.1/24 igp-weight 10");
  $cbgp->send_cmd("net node 1.0.0.1 iface 192.168.1.1 igp-weight 100");
  $cbgp->send_cmd("net node 1.0.0.3 iface 192.168.1.2 igp-weight 1000");
  $cbgp->send_cmd("net domain 1 compute");

  my $rt= cbgp_get_rt($cbgp, '1.0.0.1', '*');
  return TEST_FAILURE
    if (!check_has_route($rt, '1.0.0.2/32', -metric=>1));
  return TEST_FAILURE
    if (!check_has_route($rt, '192.168.0.0/24', -metric=>10));
  return TEST_FAILURE
    if (!check_has_route($rt, '192.168.1.0/30', -metric=>100));

  return TEST_SUCCESS;
}
