return ["bgp filter match prefix in",
	"cbgp_valid_bgp_filter_match_prefix_in"];

# -----[ cbgp_valid_bgp_filter_match_prefix_in ]---------------------
# Test ability of a filter to match route on a prefix basis.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2) virtual peer
#   - filter "prefix in 123.234/16" -> community 1:1
#   - filter "prefix in 123.235/16" -> community 1:2
#
# Topology:
#
#   R1 ----- R2
#
# Scenario:
#   * Advertise 123.234/16, 123.235/16 and 123.236/16
#   * Check that 123.234.1/24 has community 1:1
#   * Check that 123.235.1/24 has community 1:2
#   * Check that 123.236.1/24 has no community
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_match_prefix_in($) {
  my ($cbgp)= @_;

  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "prefix in 123.234/16",
	      "community add 1:1");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "prefix in 123.235/16",
	      "community add 1:2");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "123.234.1/24|2|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "123.235.1/24|2|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "123.236.1/24|2|IGP|2.0.0.1|0|0");
  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "123.234.1/24",
			     -community=>["1:1"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "123.235.1/24",
			     -community=>["1:2"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "123.236.1/24",
			     -community=>[]));
  return TEST_SUCCESS;
}

