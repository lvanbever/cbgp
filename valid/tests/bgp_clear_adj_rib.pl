return ["bgp clear-adj-rib", "cbgp_valid_bgp_clearadjrib"];

# -----[ cbgp_valid_bgp_clearadjrib ]--------------------------------
# This test first advertises a prefix within a full-mesh topology.
# It then checks that every router has the prefix in the Loc-RIB and
# the Adj-RIB-in (except for the origin).
# Then, the adj-ribs are cleared using "bgp clear-adj-rib". Every
# router should still have the route in its Loc-RIB, but all Adj-RIBs
# should be empty.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_clearadjrib($) {
  my ($cbgp)= @_;
  $topo= topo_3nodes_triangle();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  cbgp_topo_bgp_routers($cbgp, $topo, 1);
  cbgp_topo_ibgp_sessions($cbgp, $topo, 1);

  $origin= "1.0.0.1";

  $cbgp->send_cmd("bgp router $origin add network 255/8");
  $cbgp->send_cmd("sim run");

  $nodes= topo_get_nodes($topo);
  for my $router (keys(%$nodes)) {
    $rib= cbgp_get_rib($cbgp, $router);
    return TEST_FAILURE if !cbgp_rib_check($rib, "255.0.0.0/8");
    if ($router ne $origin) {
      $ribin= cbgp_get_rib_in($cbgp, $router);
      return TEST_FAILURE if !cbgp_rib_check($ribin, "255.0.0.0/8");
    }
  }

  $cbgp->send_cmd("bgp clear-adj-rib");
  for my $router (keys %$nodes) {
    $rib= cbgp_get_rib($cbgp, $router);
    return TEST_FAILURE if !cbgp_rib_check($rib, "255.0.0.0/8");
    if ($router ne $origin) {
      $ribin= cbgp_get_rib_in($cbgp, $router);
      return TEST_FAILURE if cbgp_rib_check($ribin, "255.0.0.0/8");
    }
  }

  return TEST_SUCCESS;
}

