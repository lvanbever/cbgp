return ["bgp add router (duplicate)", "cbgp_valid_bgp_add_router_dup"];

# -----[ cbgp_valid_bgp_add_router_dup ]-----------------------------
# Test that an error is returned when we try to create a BGP router
# that already exists.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_add_router_dup($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.0");
  $cbgp->send_cmd("bgp add router 1 1.0.0.0");
  # Check that adding the same router twice fails
  my $msg= cbgp_check_error($cbgp, "bgp add router 1 1.0.0.0");
  return TEST_FAILURE
    if (!check_has_error($msg, "could not add BGP router"));
  return TEST_SUCCESS;
}

