return ["bgp add network", "cbgp_valid_bgp_add_network"];

# -----[ cbgp_valid_bgp_add_network ]--------------------------------
# Test ability to add a network to a BGP router.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_add_network($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  my $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.1 add network 255/8");
  return TEST_FAILURE
    if (check_has_error($msg));
  return TEST_SUCCESS;
}

