return ["bgp router show filter", "cbgp_valid_bgp_router_show_filter"];

# -----[ cbgp_valid_bgp_router_show_filter ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_show_filter($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add node 1.0.0.0");
  $cbgp->send_cmd("bgp add router 1 1.0.0.0");
  $cbgp->send_cmd("bgp router 1.0.0.0");
  $cbgp->send_cmd("  add peer 2 2.0.0.0");
  $cbgp->send_cmd("  peer 2.0.0.0");
  $cbgp->send_cmd("    filter in");
  $cbgp->send_cmd("      add-rule");
  $cbgp->send_cmd("        match any");
  $cbgp->send_cmd("        action \"local-pref 100, metric 200\"");
  $cbgp->send_cmd("");
  $cbgp->send_cmd("bgp router 1.0.0.0 peer 2.0.0.0 filter in show");
  cbgp_checkpoint($cbgp);
  return TEST_SUCCESS;
}

