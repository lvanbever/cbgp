return ["bgp filter match as-path regex",
	"cbgp_valid_bgp_filter_match_aspath_regex"];

# -----[ cbgp_valid_bgp_filter_match_aspath_regex ]------------------
# Test ability of a filter to match route on a prefix basis.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2) virtual peer
#   - filter "path [^2 .*]" -> community 1:1
#   - filter "path [.* 254$]" -> community 1:2
#   - filter "path [.* 255$]" -> community 1:3
#
# Scenario:
#   * Advertise 253/8 [2 253], 254/8 [2 254] and 255/8 [2 255]
#   * Check that 253/8 has community 1:1
#   * Check that 254/8 has community 1:2
#   * Check that 255/8 has no community
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_match_aspath_regex($) {
  my ($cbgp)= @_;

  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0], ["3.0.0.1", 3, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "path \"^2 .*\"",
	      "community add 1:1");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "path \".* 254\\\$\"",
	      "community add 1:2");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "path \\\".* 255\$\\\"",
	      "community add 1:3");
  cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in",
	      "path \\\"^2 .*\\\"",
	      "community add 1:1");
  cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in",
	      "path \\\".* 254\$\\\"",
	      "community add 1:2");
  cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in",
	      "path \\\".* 255\$\\\"",
	      "community add 1:3");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "253.2/16|2 253|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "254.2/16|2 254|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255.2/16|2 255|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "253.3/16|3 253|IGP|3.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "254.3/16|3 254|IGP|3.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "255.3/16|3 255|IGP|3.0.0.1|0|0");

  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "253.2/16",
			     -community=>["1:1"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "254.2/16",
			     -community=>["1:1", "1:2"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.2/16",
			     -community=>["1:1", "1:3"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "253.3/16",
			     -community=>[]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "254.3/16",
			     -community=>["1:2"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.3/16",
			     -community=>["1:3"]));
  return TEST_SUCCESS;
}

