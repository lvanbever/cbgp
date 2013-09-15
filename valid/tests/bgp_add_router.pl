return ["bgp add router", "cbgp_valid_bgp_add_router"];

# -----[ cbgp_valid_bgp_add_router ]---------------------------------
# Test ability to create a BGP router.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_add_router($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.0");
  # Check that router could be added
  my $msg= cbgp_check_error($cbgp, "bgp add router 1 1.0.0.0");
  return TEST_FAILURE
    if (check_has_error($msg));
  return TEST_SUCCESS;
}

