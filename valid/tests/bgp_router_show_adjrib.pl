return ["bgp router show adj-rib", "cbgp_valid_bgp_router_show_adj_rib"];

# -----[ cbgp_valid_bgp_router_show_adj_rib ]------------------------
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_show_adj_rib($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add node 1.0.0.0");
  $cbgp->send_cmd("net add node 2.0.0.0");
  $cbgp->send_cmd("net add node 2.0.0.1");
  $cbgp->send_cmd("net add link 1.0.0.0 2.0.0.0");
  $cbgp->send_cmd("net add link 1.0.0.0 2.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.0 route add 2.0.0.0/32 --oif=2.0.0.0/32 1");
  $cbgp->send_cmd("net node 1.0.0.0 route add 2.0.0.1/32 --oif=2.0.0.1/32 2");
  $cbgp->send_cmd("bgp add router 1 1.0.0.0");
  $cbgp->send_cmd("bgp router 1.0.0.0");
  $cbgp->send_cmd("\tadd peer 2 2.0.0.0");
  $cbgp->send_cmd("\tpeer 2.0.0.0 virtual");
  $cbgp->send_cmd("\tpeer 2.0.0.0 up");
  $cbgp->send_cmd("\tpeer 2.0.0.0 recv \"BGP4|0|A|2.0.0.0|2|255/8|2|IGP|2.0.0.0|1|0|\"");
  $cbgp->send_cmd("\tadd peer 2 2.0.0.1");
  $cbgp->send_cmd("\tpeer 2.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 2.0.0.1 up");
  $cbgp->send_cmd("\tpeer 2.0.0.1 recv \"BGP4|0|A|2.0.0.1|2|255/8|2|IGP|2.0.0.1|1|0|\"");
  $cbgp->send_cmd("\texit");

  # Try to obtain adj-rib-in for peer 2.0.0.0 only
  my $rib= cbgp_get_rib_in($cbgp, '1.0.0.0', '2.0.0.0', undef);
  return TEST_FAILURE if (scalar(values %$rib) != 1);
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"2.0.0.0"));

  # Try to obtain adj-rib-in for peer 2.0.0.1 only
  $rib= cbgp_get_rib_in($cbgp, '1.0.0.0', '2.0.0.1', undef);
  return TEST_FAILURE if (scalar(values %$rib) != 1);
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"2.0.0.1"));

  # Try to get adj-rib-in with unknown peer
  my $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 show adj-rib in 1.2.3.4 *");
  return TEST_FAILURE
    if (!check_has_error($msg, "unknown peer"));

  # Try to get adj-rib-in with invalid peer address
  $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 show adj-rib in 1.2.a.4 *");
  return TEST_FAILURE
    if (!check_has_error($msg, "invalid peer address"));

  # Try to get adj-rib with invalid direction
  $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 show adj-rib xxx * *");
  return TEST_FAILURE
    if (!check_has_error($msg, "invalid adj-rib side"));

  # Try to get adj-rib with an invalid prefix
  $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 show adj-rib in * xxx");
  return TEST_FAILURE
    if (!check_has_error($msg, "invalid prefix\|address\|[*]"));

  # Try to get adj-rib with a valid prefix
  $rib= cbgp_get_rib_in($cbgp, '1.0.0.0', '2.0.0.1', '255/8');
  return TEST_FAILURE if (scalar(values %$rib) != 1);
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"2.0.0.1"));

  return TEST_SUCCESS;
}
