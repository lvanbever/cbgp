return ["net node duplicate", "cbgp_valid_net_node_duplicate"];

# -----[ cbgp_valid_net_node_duplicate ]-----------------------------
# Test ability to detect creation of duplicated nodes.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Try to add node 1.0.0.1
#   * Check that an error is returned (and check error message)
# -------------------------------------------------------------------
sub cbgp_valid_net_node_duplicate($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");

  my $msg= cbgp_check_error($cbgp, "net add node 1.0.0.1");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_NODE_EXISTS));
  return TEST_SUCCESS;
}

