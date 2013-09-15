return ["bgp topology load (addr-sch=local)", "cbgp_valid_bgp_topology_load_local"];

# -----[ cbgp_valid_bgp_topology_load_local ]------------------------
# Test different addressing scheme for "bgp topology load".
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_local($) {
  my ($cbgp)= @_;

  my $topo_file= get_resource("valid-bgp-topology.subramanian");
  (-e $topo_file) or return TEST_DISABLED;

  my $topo= topo_from_subramanian_file($topo_file, "local");

  $cbgp->send_cmd("bgp topology load --addr-sch=local \"$topo_file\"");
  $cbgp->send_cmd("bgp topology install");

  # Check that all links are created
  (!cbgp_topo_check_links($cbgp, $topo)) and
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

