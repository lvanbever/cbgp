return ["net link missing", "cbgp_valid_net_link_missing"];

# -----[ cbgp_valid_net_link_missing ]-------------------------------
# Test ability to detect creation of link with missing destination.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Try to add link to 1.0.0.2 (missing)
#   * Check that an error is returned (and check error message)
# -------------------------------------------------------------------
sub cbgp_valid_net_link_missing($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");

  my $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 1.0.0.2");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_INVALID_LINK_TAIL));
  return TEST_SUCCESS;
}

