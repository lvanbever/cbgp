return ["bgp topology subra-2004", "cbgp_valid_bgp_topology_subra2004"];

# -----[ cbgp_valid_bgp_topology_subra2004 ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_subra2004($) {
  my ($cbgp)= @_;
  my $topo_file= get_resource("data/subra-2004.txt");
  (-e $topo_file) or return TEST_DISABLED;

  my $topo= topo_from_subramanian_file($topo_file);
  $cbgp->send_cmd("bgp topology load \"$topo_file\"");
  $cbgp->send_cmd("bgp topology install");

  # Check that all links are created
  (!cbgp_topo_check_links($cbgp, $topo)) and
    return TEST_FAILURE;

  # Check the propagation of two prefixes
  $cbgp->send_cmd("bgp topology policies");
  $cbgp->send_cmd("bgp topology run");
  $cbgp->send_cmd("sim run");
  # AS7018
  $cbgp->send_cmd("bgp router AS7018 add network 255/8");
  # AS8709
  $cbgp->send_cmd("bgp router AS8709 add network 245/8");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib($cbgp, "AS7018");
  $rib= cbgp_get_rib($cbgp, "AS8709");
  return TEST_SUCCESS;
}

