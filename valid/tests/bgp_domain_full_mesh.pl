return ["bgp domain full-mesh",
	"cbgp_valid_bgp_domain_fullmesh",
	topo_3nodes_triangle()];

# -----[ cbgp_valid_bgp_domain_fullmesh ]----------------------------
# Test ability to create a full-mesh of iBGP sessions between all the
# routers of a domain.
#
# Topology:
#
#   R1 ---- R2
#     \    /
#      \  /
#       R3
#
# Scenario:
#   * Create a topology composed of nodes R1, R2 and R3
#   * Create BGP routers with R1, R2 and R3 in domain 1
#   * Run 'bgp domain 1 full-mesh'
#   * Check setup of sessions
#   * Check establishment of sessions
# -------------------------------------------------------------------
sub cbgp_valid_bgp_domain_fullmesh($$) {
  my ($cbgp, $topo)= @_;
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  cbgp_topo_bgp_routers($cbgp, $topo, 1);
  $cbgp->send_cmd("bgp domain 1 full-mesh");
  if (!cbgp_topo_check_ibgp_sessions($cbgp, $topo, C_PEER_OPENWAIT)) {
    return TEST_FAILURE;
  }
  $cbgp->send_cmd("sim run");
  if (!cbgp_topo_check_ibgp_sessions($cbgp, $topo, C_PEER_ESTABLISHED)) {
    return TEST_FAILURE;
  }
  if (!cbgp_topo_check_bgp_networks($cbgp, $topo)) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

