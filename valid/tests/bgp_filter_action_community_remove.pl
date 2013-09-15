return ["bgp filter action community remove",
	"cbgp_valid_bgp_filter_action_community_remove"];

# -----[ cbgp_valid_bgp_filter_action_community_remove ]-------------
# Test ability of a filter to remove a single community in a route.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2) virtual peer
#   - R3 (3.0.0.1, AS3) virtual peer
#   - R4 (4.0.0.1, AS4) virtual peer
#   - filter [from R2] "any" -> "community remove 2:2"
#   - filter [from R3] "any" -> "community remove 3:1"
#   - filter [from R3] "any" -> "community remove 4:1, community remove 4:2"
#
# Topology:
#
#   (R2) --*
#           \
#            \
#   (R3) ---- R1
#            /
#           /
#   (R4) --*
#
# Scenario:
#   * Advertised prefixes: 253/8 [2:1, 2:2, 2:3] from R2,
#                          254/8 [3:5] from R3,
#                          255/8 [4:0, 4:1, 4:2] from R4
#   * Check that 253/8 has [2:1, 2:3]
#   * Check that 254/8 has [3:5]
#   * Check that 255/8 has [4:0]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_community_remove($) {
  my ($cbgp)= @_;

  cbgp_topo_dp3($cbgp,
		["2.0.0.1", 2, 0],
		["3.0.0.1", 3, 0],
		["4.0.0.1", 4, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "any", "community remove 2:2");
  cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in",
	      "any", "community remove 3:1");
  cbgp_filter($cbgp, "1.0.0.1", "4.0.0.1", "in",
	      "any", "community remove 4:1, community remove 4:2");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "253/8|2|IGP|2.0.0.1|0|0|2:1 2:2 2:3");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "254/8|3|IGP|3.0.0.1|0|0|3:5");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "4.0.0.1",
		   "255/8||IGP|4.0.0.1|0|0|4:0 4:1 4:2");

  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "253/8",
			     -community=>["2:1", "2:3"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "254/8",
			     -community=>["3:5"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>["4:0"]));
  return TEST_SUCCESS;
}

