return ["bgp filter action community add",
	"cbgp_valid_bgp_filter_action_community_add"];

# -----[ cbgp_valid_bgp_filter_action_community_add ]----------------
# Test ability to attach communities to routes.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_community_add() {
  my ($cbgp)= @_;
  cbgp_topo_filter($cbgp);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "community add 1:2");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/16|2|IGP|2.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  my $rib;
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.0/16",
			     -community=>["1:2"]));
  return TEST_SUCCESS;
}

