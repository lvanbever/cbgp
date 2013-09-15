return ["net igp external", "cbgp_valid_net_igp_external"];

# -----[ cbgp_valid_net_igp_external ]-------------------------------
# Description:
# This test checks how links accross the boundaries of an IGP domain
# are handled. For this purpose, a node R1 is placed at the border of
# IGP domain 1. Inside domain 1, R1 is connected to R2 with a simple
# link. Outside domain 1, R1 is connected to R3 through 3 different
# links: an rtr link, a ptp link and a ptmp link (through a subnet).
#
# Setup:
#   - R1 (1.0.0.0)
#   - R2 (1.0.0.1)
#   - R3 (2.0.0.0)
#   - N1 (10.0.0.0/24, transit)
#   - N2 (192.168.0/30)
#   - Domain 1 = {R1, R2}
#
# Topology:
#                 .1  N2  .2
#                 *--------*
#                /          \
#     R2 ----- R1 ---------- R3
#                \          /
#                 *-- N1 --*
#
# Scenario:
#   * Compute IGP routes
#   * Check that R1 has a route towards N1, N2 and R2
#   * Check that R1 has no route towards R3
#   * Check that R2 has a route towards R1, N1 and N2 through R1
#   * Check that R2 has no route towards R3
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_external($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.0");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 2.0.0.0");
  $cbgp->send_cmd("net add subnet 10.0.0.0/24 transit");
  $cbgp->send_cmd("net add link 1.0.0.0 1.0.0.1");
  $cbgp->send_cmd("net add link 1.0.0.0 2.0.0.0");
  $cbgp->send_cmd("net add link-ptp 1.0.0.0 192.168.0.1/30 2.0.0.0 192.168.0.2/30");
  $cbgp->send_cmd("net add link 1.0.0.0 10.0.0.1/24");
  $cbgp->send_cmd("net add link 2.0.0.0 10.0.0.2/24");

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 1.0.0.0 domain 1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net link 1.0.0.0 1.0.0.1 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.0 2.0.0.0 igp-weight 1");
  $cbgp->send_cmd("net node 1.0.0.0 iface 192.168.0.1/30 igp-weight 1");
  $cbgp->send_cmd("net node 2.0.0.0 iface 192.168.0.2/30 igp-weight 1");
  $cbgp->send_cmd("net link 1.0.0.0 10.0.0.1/24 igp-weight 1");
  $cbgp->send_cmd("net link 2.0.0.0 10.0.0.2/24 igp-weight 1");
  $cbgp->send_cmd("net domain 1 compute");

  # Check routes in node R1
  my $rt= cbgp_get_rt($cbgp, '1.0.0.0');
  return TEST_FAILURE
    if (!check_has_route($rt, '1.0.0.1/32'));
  return TEST_FAILURE
    if (!check_has_route($rt, '192.168.0.0/30'));
  return TEST_FAILURE
    if (!check_has_route($rt, '10.0.0.0/24'));
  return TEST_FAILURE
    if (check_has_route($rt, '2.0.0.0/32'));

  # Check routes in node R2
  $rt= cbgp_get_rt($cbgp, '1.0.0.1');
  return TEST_FAILURE
    if (!check_has_route($rt, '1.0.0.0/32',
			 -iface=>'1.0.0.0'));
  return TEST_FAILURE
    if (!check_has_route($rt, '192.168.0.0/30',
			 -iface=>'1.0.0.0'));
  return TEST_FAILURE
    if (!check_has_route($rt, '10.0.0.0/24',
			 -iface=>'1.0.0.0'));
  return TEST_FAILURE
    if (check_has_route($rt, '2.0.0.0/32'));

  return TEST_SUCCESS;
}
