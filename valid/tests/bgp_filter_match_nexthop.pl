return ["bgp filter match next-hop",
	"cbgp_valid_bgp_filter_match_nexthop"];

# -----[ cbgp_valid_bgp_filter_match_nexthop ]-----------------------
# -------------------------------------------------------------------
sub cbgp_valid_bgp_filter_match_nexthop($) {
  my ($cbgp)= @_;
  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0], ["2.0.0.2", 2, 0],
		["2.0.0.3", 2, 0], ["2.0.1.4", 2, 0]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "(next-hop is 2.0.0.2)", "community add 1:2");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "(next-hop is 2.0.0.3)", "community add 1:3");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "(next-hop in 2.0.0/24)", "community add 1:2000");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "(next-hop in 2.0.1/24)", "community add 1:2001");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2|IGP|2.0.0.2|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "254/8|2|IGP|2.0.0.3|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "253/8|2|IGP|2.0.1.4|0|0");
  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>["1:2", "1:2000"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "254/8",
			     -community=>["1:3", "1:2000"]));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "253/8",
			     -community=>["1:2001"]));
  return TEST_SUCCESS;
}

