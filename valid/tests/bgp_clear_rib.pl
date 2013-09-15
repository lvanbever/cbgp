return ["bgp clear-rib", "cbgp_valid_bgp_clearrib"];

# -----[ cbgp_valid_bgp_clearrib ]-----------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_clearrib($) {
  my ($cbgp)= @_;
  $topo= topo_3nodes_triangle();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  cbgp_topo_bgp_routers($cbgp, $topo, 1);
  cbgp_topo_ibgp_sessions($cbgp, $topo, 1);

  $cbgp->send_cmd("bgp router 1.0.0.1 add network 255/8");
  $cbgp->send_cmd("sim run");

  $nodes= topo_get_nodes($topo);
  for my $router (keys(%$nodes)) {
    $rib= cbgp_get_rib($cbgp, $router);
    return TEST_FAILURE if !cbgp_rib_check($rib, "255.0.0.0/8");
  }

  $cbgp->send_cmd("bgp clear-rib");
  for my $router (keys %$nodes) {
    $rib= cbgp_get_rib($cbgp, $router);
    return TEST_FAILURE if cbgp_rib_check($rib, "255.0.0.0/8");
  }

  return TEST_SUCCESS;
}

