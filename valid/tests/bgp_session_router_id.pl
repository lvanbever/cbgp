return ["bgp session router-id", "cbgp_valid_bgp_session_router_id"];

# -----[ cbgp_valid_bgp_session_router_id ]--------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_router_id($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 3 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");

  return TEST_SUCCESS;
}

