return ["net longest-matching", "cbgp_valid_net_longest_matching"];

# -----[ cbgp_valid_net_longest_matching ]---------------------------
# Check ability to forward along route with longest-matching prefix.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#
# Topology:
#
#   R1 ----- R2 ----- R3
#
# Scenario:
#   * Add route towards 1/8 with next-hop R1 in R2
#     Add route towards 1.1/16 with next-hop R3 in R2
#   * Perform a record-route from R2 to 1.0.0.0,
#     checks that first hop is R1
#   * Perform a record-route from R2 to 1.1.0.0,
#     checks that first hop is R3
# -------------------------------------------------------------------
sub cbgp_valid_net_longest_matching($) {
  my ($cbgp)= @_;
  my $topo= topo_3nodes_line();
  cbgp_topo($cbgp, $topo);
  $cbgp->send_cmd("net node 1.0.0.2 route add --oif=1.0.0.1 1/8 10");
  $cbgp->send_cmd("net node 1.0.0.2 route add --oif=1.0.0.3 1.1/16 10");

  # Test longest-matching in show-rt
  my $rt;
  $rt= cbgp_get_rt($cbgp, "1.0.0.2", "1.0.0.0");
  return TEST_FAILURE
    if (!check_has_route($rt, "1/8", -iface=>"1.0.0.1"));
  $rt= cbgp_get_rt($cbgp, "1.0.0.2", "1.1.0.0");
  return TEST_FAILURE
    if (!check_has_route($rt, "1.1/16", -iface=>"1.0.0.3"));
  # Test longest-matching in record-route (only first hop)
  my $trace;
  $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.0.0.0");
  return TEST_FAILURE
    if (!check_recordroute($trace, -status=>"UNREACH",
			   -path=>[[1,"1.0.0.1"]]));
  $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.1.0.0");
  return TEST_FAILURE
    if (!check_recordroute($trace, -status=>"UNREACH",
			   -path=>[[1,"1.0.0.3"]]));
  return TEST_SUCCESS;
}

