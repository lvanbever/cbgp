return ["bgp router add peer (self)", "cbgp_valid_bgp_router_add_peer_self"];

# -----[ cbgp_valid_bgp_router_add_peer_self ]-----------------------
# Test that an error is returned when we try to add a BGP neighbor
# with an address that is owned by the target router..
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_add_peer_self($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.0");
  $cbgp->send_cmd("bgp add router 1 1.0.0.0");
  my $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.0 add peer 1 1.0.0.0");
  return TEST_FAILURE
    if (!check_has_error($msg, "peer already exists"));
  return TEST_SUCCESS;
}

