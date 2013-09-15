return ["bgp filter match community",
	"cbgp_valid_bgp_filter_match_community"];

# -----[ cbgp_valid_bgp_filter_match_community ]---------------------
sub cbgp_valid_bgp_filter_match_community() {
  my ($cbgp)= @_;
  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "community is 1:100",
	      "local-pref 100");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "community is 1:80",
	      "local-pref 80");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in",
	      "community is 1:60",
	      "local-pref 60");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2|IGP|2.0.0.1|0|0|1:100");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "254/8|2|IGP|2.0.0.1|0|0|1:80");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "253/8|2|IGP|2.0.0.1|0|0|1:60");
  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8", -pref=>100));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "254/8", -pref=>80));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "253/8", -pref=>60));

  return TEST_SUCCESS;
}

