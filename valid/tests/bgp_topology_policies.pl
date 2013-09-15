return ["bgp topology policies", "cbgp_valid_bgp_topology_policies"];

# -----[ cbgp_valid_bgp_topology_policies ]--------------------------
# Test ability to load AS-level topology and to define route filters
# based on the given business relationships.
#
# Setup:
#   see 'valid bgp topology load'
#
# Scenario:
#
# Resources:
#   [valid-bgp-topology.subramanian]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_policies($) {
  my ($cbgp)= @_;

  my $topo_file= get_resource("valid-bgp-topology.subramanian");
  (-e $topo_file) or return TEST_SKIPPED;

  $cbgp->send_cmd("bgp topology load \"$topo_file\"");
  $cbgp->send_cmd("bgp topology install");
  $cbgp->send_cmd("bgp topology policies");

  return TEST_SUCCESS;
}

