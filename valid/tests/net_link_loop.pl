return ["net link loop", "cbgp_valid_net_link_loop"];

# -----[ cbgp_valid_net_link_loop ]----------------------------------
# Test that C-BGP will return an error message if one tries to create
# a link with equal source and destination.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Try to create link 1.0.0.1 <--> 1.0.0.1
#   * Check that an error is returned (and check error message)
# -------------------------------------------------------------------
sub cbgp_valid_net_link_loop($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");

  my $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 1.0.0.1");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_LINK_ENDPOINTS));
  return TEST_SUCCESS;
}

