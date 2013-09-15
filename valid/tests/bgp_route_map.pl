return ["bgp route-map", "cbgp_valid_bgp_route_map"];

# -----[ cbgp_valid_bgp_route_map ]----------------------------------
# Test the ability to define route-maps (i.e. filters that can be
# shared by multiple BGP sessions.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2, virtual)
#
# Topology:
#
#   R1 ----- (R2)
#
# Scenario:
#   * Define a route-map named RM_LP_100 that changes the local-pref
#     of routes matching prefix 128/8
#   * Define a route-map named RM_LP_200 that changes the local-pref
#     of routes matching prefix 0/8
#   * Assign both route-maps as input filter of R1 for routes
#     received from R2
#   * Inject a route towards 128/8 in R1 from R2
#   * Inject a route towards 0/8 in R1 from R2
#   * Check that the local-pref is as follows
#       128/8 -> LP=100
#       0/8   -> LP=200
# -------------------------------------------------------------------
sub cbgp_valid_bgp_route_map($) {
  my ($cbgp)= @_;

  # Define two route-maps
  $cbgp->send_cmd("bgp route-map RM_LP_100");
  $cbgp->send_cmd("  add-rule");
  $cbgp->send_cmd("    match \"prefix in 128/8\"");
  $cbgp->send_cmd("    action \"local-pref 100\"");
  $cbgp->send_cmd("    exit");
  $cbgp->send_cmd("  exit");
  $cbgp->send_cmd("bgp route-map RM_LP_200");
  $cbgp->send_cmd("  add-rule");
  $cbgp->send_cmd("    match \"prefix in 0/8\"");
  $cbgp->send_cmd("    action \"local-pref 200\"");
  $cbgp->send_cmd("    exit");
  $cbgp->send_cmd("  exit");

  # Create one BGP router and injects
  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "any",
	      "call RM_LP_100");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "any",
	      "call RM_LP_200");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "128/8|2|IGP|2.0.0.1|0|0|");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "0/8|2|IGP|2.0.0.1|0|0|");
  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "128/8", -pref=>100));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "0/8", -pref=>200));

  return TEST_SUCCESS;
}

