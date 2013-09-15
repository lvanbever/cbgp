return ["bgp add network (duplicate)", "cbgp_valid_bgp_add_network_dup"];

# -----[ cbgp_valid_bgp_add_network_dup ]----------------------------
# Test that an error is returned when an attempt is made at adding
# a network twice to the same router.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_add_network_dup($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1 add network 255/8");
  my $msg= cbgp_check_error($cbgp, "bgp router 1.0.0.1 add network 255/8");
  return TEST_FAILURE
    if (!check_has_error($msg, "network already exists"));
  return TEST_SUCCESS;
}

