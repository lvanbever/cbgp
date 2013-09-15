return ["bgp filter action med",
	"cbgp_valid_bgp_filter_action_med"];

# -----[ cbgp_valid_bgp_filter_action_med ]--------------------------
# Test ability to attach a specific MED value to routes.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2)
#   - filter [from R2] "any" -> "metric 57"
#
# Topology:
#
#   (R2) ----- R1
#
# Scenario:
#   * Advertised route 255/16 [metric:0]
#   * Check that route has MED value set to 57
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_med() {
  my ($cbgp)= @_;

  cbgp_topo_filter($cbgp);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "any", "metric 57");
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		  "\"BGP4|0|A|1.0.0.1|1|255/16|2|IGP|2.0.0.1|0|0\"");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.0/16",
			     -med=>57));
  return TEST_SUCCESS;
}

