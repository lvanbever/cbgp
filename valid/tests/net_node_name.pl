return ["net node name", "cbgp_valid_net_node_name"];

# -----[ cbgp_valid_net_node_name ]----------------------------------
# Test ability to associate a name with a node.
#
# Scenario:
#   * Create a node 1.0.0.1
#   * Read its name (should be empty)
#   * Change its name to "R1"
#   * Read its name (should be "R1")
# -------------------------------------------------------------------
sub cbgp_valid_net_node_name($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  my $info= cbgp_node_info($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_node_info($info, -name=>undef));
  $cbgp->send_cmd("net node 1.0.0.1 name R1");
  $info= cbgp_node_info($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_node_info($info, -name=>"R1"));
  return TEST_SUCCESS;
}

