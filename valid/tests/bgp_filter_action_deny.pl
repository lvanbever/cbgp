return ["bgp filter action deny",
	"cbgp_valid_bgp_filter_action_deny"];

# -----[ cbgp_valid_bgp_filter_action_deny ]-------------------------
# Test ability to deny a route.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_deny($) {
  my ($cbgp)= @_;
  cbgp_topo_filter($cbgp);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "deny");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/16|2|IGP|2.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255/16"));
  return TEST_SUCCESS;
}


