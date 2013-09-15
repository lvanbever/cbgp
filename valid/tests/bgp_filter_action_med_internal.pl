return ["bgp filter action med internal",
	"cbgp_valid_bgp_filter_action_med_internal"];

# -----[ cbgp_valid_bgp_filter_action_med_internal ]-----------------
# Test ability to attach a MED value to routes, based on the IGP
# cost.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) virtual peer of R1 (IGP cost:5)
#   - R3 (1.0.0.3, AS1) virtual peer of R1 (IGP cost:10)
#   - R4 (2.0.0.1, AS2) peer of R1
#   - filter [to R2] "any" -> "metric internal"
#
# Topology:
#
#   (R2) ---*
#            \
#             R1 ---- R4
#            /
#   (R3) ---*
#
# Scenario:
#   * Advertise 254/8 from R2, 255/8 from R3
#   * Check that R4 has 254/8 with MED:5 and 255/8 with MED:10
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_med_internal() {
  my ($cbgp)= @_;
  cbgp_topo_dp4($cbgp,
		["1.0.0.2", 1, 5, 1],
		["1.0.0.3", 1, 10, 1],
		["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "out",
	      "any", "metric internal");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "254/8|254|IGP|1.0.0.2|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		   "255/8|255|IGP|1.0.0.3|0|0");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib($cbgp, "2.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "254/8", -med=>5));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8", -med=>10));
  return TEST_SUCCESS;
}

