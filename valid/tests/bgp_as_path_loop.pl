return ["bgp as-path loop", "cbgp_valid_bgp_aspath_loop"];

# -----[ cbgp_valid_bgp_aspath_loop ]--------------------------------
# Test that a route with an AS-path that already contains the local AS
# will be filtered on input.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2)
#
# Topology:
#
#   R2 ----- R1
#
# Scenario:
#   * R2 advertises a route towards 255/8 to R1, with an AS-Path that
#     already contains AS1
#   * Check that R1 has filtered the route
# -------------------------------------------------------------------
sub cbgp_valid_bgp_aspath_loop($) {
  my ($cbgp)= @_;
  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2 1|IGP|2.0.0.1|0|0");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255/8"));

  return TEST_SUCCESS;
}

