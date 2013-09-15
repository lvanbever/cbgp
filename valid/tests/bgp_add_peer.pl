return ["bgp router add peer", "cbgp_valid_bgp_router_add_peer"];

# -----[ cbgp_valid_bgp_router_add_peer ]----------------------------
# Test ability to add a neighbor (peer) to a BGP router.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_add_peer($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.0");
  $cbgp->send_cmd("bgp add router 1 1.0.0.0");
  my $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 add peer 2 2.0.0.0");
  return TEST_FAILURE
    if (check_has_error($msg));
  return TEST_SUCCESS;
}

