return ["net node traffic load", "cbgp_valid_net_node_traffic_load"];

# -----[ cbgp_valid_net_node_traffic_load ]--------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_node_traffic_load($) {
  my ($cbgp)= @_;
  my $filename= "";
  $cbgp->send_cmd("net traffic load \"$filename\"");
  return TEST_SKIPPED;
}
