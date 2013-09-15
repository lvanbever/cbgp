return ["bgp router add peer (duplicate)", "cbgp_valid_bgp_router_add_peer_dup"];

# -----[ cbgp_valid_bgp_router_add_peer_dup ]------------------------
# Test that an error is returned when we try to add a BGP neighbor
# that already exists.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_add_peer_dup($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add node 1.0.0.0");
  $cbgp->send_cmd("bgp add router 1 1.0.0.0");
  $cbgp->send_cmd("bgp router 1.0.0.0 add peer 2 2.0.0.0");
  my $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 add peer 2 2.0.0.0");
  return TEST_FAILURE
    if (!check_has_error($msg, "peer already exists"));
  return TEST_SUCCESS;
}

