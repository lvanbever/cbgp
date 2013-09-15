return ["bgp filter action local-pref",
	"cbgp_valid_bgp_filter_action_localpref"];

# -----[ cbgp_valid_bgp_filter_action_localpref ]--------------------
# Test ability to set the local-pref value of routes.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_localpref($) {
  my ($cbgp)= @_;
  cbgp_topo_filter($cbgp);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "local-pref 57");
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		  "\"BGP4|0|A|1.0.0.1|1|255/16|2|IGP|2.0.0.1|0|0\"");
  $cbgp->send_cmd("sim run");
  my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.0/16",
			     -pref=>57));
  return TEST_SUCCESS;
}

