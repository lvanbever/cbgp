return ["bgp topology load (format=caida)", "cbgp_valid_bgp_topology_load_caida"];

# -----[ cbgp_valid_bgp_topology_load_caida ]------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_caida($) {
  my ($cbgp)= @_;
  my $filename= get_resource("as-rel.20070416.a0.01000.txt");
  (-e $filename) or return TEST_DISABLED;

  my $topo= topo_from_caida($filename);
  my $options= "--format=caida";

  $cbgp->send_cmd("bgp topology load $options \"$filename\"");
  $cbgp->send_cmd("bgp topology install");

  # Check that all links are created
  (!cbgp_topo_check_links($cbgp, $topo)) and
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

