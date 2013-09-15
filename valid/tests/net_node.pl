return ["net node", "cbgp_valid_net_node"];

# -----[ cbgp_valid_net_node ]---------------------------------------
# Test ability to create new nodes.
#
# Scenario:
#   * Create node 1.0.0.1
#   * Check that no error is issued
#   * Create node 123.45.67.89
#   * Check that no error is issued
#
#   TO-DO: read node info...
# -------------------------------------------------------------------
sub cbgp_valid_net_node($) {
  my ($cbgp)= @_;

  my $msg= cbgp_check_error($cbgp, "net add node 1.0.0.1");
  return TEST_FAILURE
    if (check_has_error($msg));
  $msg= cbgp_check_error($cbgp, "net add node 123.45.67.89");
  return TEST_FAILURE
    if (check_has_error($msg));
  return TEST_SUCCESS;
}

