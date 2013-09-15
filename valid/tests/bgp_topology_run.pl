return ["bgp topology run", "cbgp_valid_bgp_topology_run"];

# -----[ cbgp_valid_bgp_topology_run ]-------------------------------
# Setup:
#   see 'valid bgp topology load'
#
# Scenario:
#
# Resources:
#   [valid-bgp-topology.subramanian]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_run($) {
  my ($cbgp)= @_;

  my $topo_file= get_resource("valid-bgp-topology.subramanian");
  (-e $topo_file) or return TEST_DISABLED;

  my $topo= topo_from_subramanian_file($topo_file);

  $cbgp->send_cmd("bgp topology load \"$topo_file\"");
  $cbgp->send_cmd("bgp topology install");
  $cbgp->send_cmd("bgp topology policies");
  $cbgp->send_cmd("bgp topology run");
  if (!cbgp_topo_check_ebgp_sessions($cbgp, $topo, C_PEER_OPENWAIT)) {
    return TEST_FAILURE;
  }
  $cbgp->send_cmd("sim run");
  if (!cbgp_topo_check_ebgp_sessions($cbgp, $topo, C_PEER_ESTABLISHED)) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

