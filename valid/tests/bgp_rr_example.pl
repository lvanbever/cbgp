return ["bgp RR example", "cbgp_valid_bgp_rr_example"];

# -----[ cbgp_valid_bgp_rr_example ]---------------------------------
# Check that
# - a route received from an rr-client is reflected to all the peers
# - a route received from a non rr-client is only reflected to the
#   rr-clients
# - a route received over eBGP is propagated to all the peers
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_example($) {
  my ($cbgp)= @_;

  my $topo= topo_star(6);
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("\tadd network 255/8");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
  $cbgp->send_cmd("\tpeer 1.0.0.4 up");
  $cbgp->send_cmd("\tadd peer 2 1.0.0.5");
  $cbgp->send_cmd("\tpeer 1.0.0.5 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.6");
  $cbgp->send_cmd("\tpeer 1.0.0.6 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.6 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 1 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 1 1.0.0.4");
  $cbgp->send_cmd("bgp router 1.0.0.4");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 2 1.0.0.5");
  $cbgp->send_cmd("bgp router 1.0.0.5");
  $cbgp->send_cmd("\tadd network 254/8");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 1 1.0.0.6");
  $cbgp->send_cmd("bgp router 1.0.0.6");
  $cbgp->send_cmd("\tadd network 253/8");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("sim run");

  # Peer 1.0.0.1 must have received all routes
  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  (cbgp_rib_check($rib, "255/8") &&
   cbgp_rib_check($rib, "254/8") &&
   cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;

  # RR must have received all routes
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.2");
  (cbgp_rib_check($rib, "255/8") &&
   cbgp_rib_check($rib, "254/8") &&
   cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;

  # RR-client 1.0.0.3 must have received all routes
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.3");
  (cbgp_rib_check($rib, "255/8") &&
   cbgp_rib_check($rib, "254/8") &&
   cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;

  # Peer 1.0.0.4 must only have received the 254/8 and 253/8 routes
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.4");
  (!cbgp_rib_check($rib, "255/8") &&
   cbgp_rib_check($rib, "254/8") &&
   cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;

  # External peer 1.0.0.5 must have received all routes
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.5");
  (cbgp_rib_check($rib, "255/8") &&
   cbgp_rib_check($rib, "254/8") &&
   cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;

  # RR-client 1.0.0.6 must have received all routes
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.6");
  (cbgp_rib_check($rib, "255/8") &&
   cbgp_rib_check($rib, "254/8") &&
   cbgp_rib_check($rib, "253/8")) or return TEST_FAILURE;

  return TEST_SUCCESS;
}

