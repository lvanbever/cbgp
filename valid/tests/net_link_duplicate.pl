return ["net link duplicate", "cbgp_valid_net_link_duplicate"];

# -----[ cbgp_valid_net_link_duplicate ]-----------------------------
# Test that C-BGP will return an error message if one tries to create
# a link with equal source and destination.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Try to create link 1.0.0.1 <--> 1.0.0.1
#   * Check that an error is returned (and check error message)
# -------------------------------------------------------------------
sub cbgp_valid_net_link_duplicate($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");

  my $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 1.0.0.2");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_LINK_EXISTS));
  return TEST_SUCCESS;
}

