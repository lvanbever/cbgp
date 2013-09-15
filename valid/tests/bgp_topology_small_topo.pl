return ["bgp topology small-topo", "cbgp_valid_bgp_topology_smalltopo"];

# -----[ cbgp_valid_bgp_topology_smalltopo ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_smalltopo($) {
  my ($cbgp)= @_;
  my $topo_file= get_resource("data/small.topology-nz.txt");
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
  # AS1
  $cbgp->send_cmd("bgp router AS1 add network 255/8");
  # AS11923
  $cbgp->send_cmd("bgp router AS11923 add network 254/8");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib($cbgp, "AS1");
  $rib= cbgp_get_rib($cbgp, "AS11923");
  return TEST_SUCCESS;
}


