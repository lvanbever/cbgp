return ["bgp filter action community strip",
	"cbgp_valid_bgp_filter_action_community_strip"];

# -----[ cbgp_valid_bgp_filter_action_community_strip ]--------------
# Test ability of a filter to remove all communities from a route.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2) virtual peer
#   - filter [from R2] "any" -> "community strip"
#
# Topology:
#
#   (R2) ----- R1
#
# Scenario:
#   * Advertised prefix 255/8 [2:1 2:255] from R2
#   * Check that 255/8 does not contain communities
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_action_community_strip($) {
  my ($cbgp)= @_;

  cbgp_topo_dp3($cbgp,
		["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "any", "community strip");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2|IGP|2.0.0.1|0|0|2:1 2:255");

  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>[]));
  return TEST_SUCCESS;
  }

