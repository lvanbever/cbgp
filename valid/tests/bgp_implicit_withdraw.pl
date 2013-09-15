return ["bgp implicit-withdraw",
	"cbgp_valid_bgp_implicit_withdraw"];

# -----[ cbgp_valid_bgp_implicit_withdraw ]--------------------------
# Test the replacement of a previously advertised route by a new one,
# without explicit withdraw.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (2.0.0.1, AS2)
#   - R4 (2.0.0.2, AS2)
#
# Topology:
#
#   (R3) --*
#           \
#            R1 ----- R2
#           /
#   (R4) --*
#
# Scenario:
#   * R3 announces to R1 a route towards 255/8, with AS-Path=2 3 4
#   * R4 announces to R1 a route towards 255/8, with AS-Path=2 4
#   * Check that R1 has selected R4's route
#   * R3 announces to R1 a route towards 255/8, with AS-Path=2 4
#   * Check that R1 has selected R3's route
# -------------------------------------------------------------------
sub cbgp_valid_bgp_implicit_withdraw($) {
  my ($cbgp)= @_;
  cbgp_topo_dp3($cbgp,
		["1.0.0.2", 1, 10],
		["2.0.0.1", 2, 0],
		["2.0.0.2", 2, 0]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2 3 4|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.2",
		   "255/8|2 4|IGP|2.0.0.2|0|0");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"2.0.0.2"));
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2 4|IGP|2.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"2.0.0.1"));
  return TEST_SUCCESS;
}
