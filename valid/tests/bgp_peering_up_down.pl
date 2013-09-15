return ["bgp peering up/down", "cbgp_valid_bgp_peering_up_down"];

# -----[ cbgp_valid_bgp_peering_up_down ]----------------------------
# Test ability to shutdown and re-open BGP sessions from the CLI.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#
# Topology:
#
#   R1 ----- R2
#
# Scenario:
#   * Originate prefix 255.255/16 in R1
#   * Originate prefix 255.254/16 in R2
#   * Check routes in R1 and R2
#   * Shutdown session between R1 and R2
#   * Check routes in R1 and R2
#   * Re-open session between R1 and R2
#   * Check routes in R1 and R2
# -------------------------------------------------------------------
sub cbgp_valid_bgp_peering_up_down($) {
  my ($cbgp)= @_;
  my $topo= topo_2nodes();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  cbgp_topo_bgp_routers($cbgp, $topo, 1);
  $cbgp->send_cmd("bgp domain 1 full-mesh");
  $cbgp->send_cmd("bgp router 1.0.0.1 add network 255.255.0.0/16");
  $cbgp->send_cmd("bgp router 1.0.0.2 add network 255.254.0.0/16");
  $cbgp->send_cmd("sim run");
  if (!cbgp_check_bgp_route($cbgp, "1.0.0.1", "255.254.0.0/16") ||
      !cbgp_check_bgp_route($cbgp, "1.0.0.2", "255.255.0.0/16")) {
    return TEST_FAILURE;
  }
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 down");
  $cbgp->send_cmd("sim run");
  if (cbgp_check_bgp_route($cbgp, "1.0.0.1", "255.254.0.0/16") ||
      cbgp_check_bgp_route($cbgp, "1.0.0.2", "255.255.0.0/16")) {
    return TEST_FAILURE;
  }
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 up");
  $cbgp->send_cmd("sim run");
  if (!cbgp_check_bgp_route($cbgp, "1.0.0.1", "255.254.0.0/16") ||
      !cbgp_check_bgp_route($cbgp, "1.0.0.2", "255.255.0.0/16")) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

