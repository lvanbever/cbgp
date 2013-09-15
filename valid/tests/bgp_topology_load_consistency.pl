return ["bgp topology load (consistency)", "cbgp_valid_bgp_topology_load_consistency"];

# -----[ cbgp_valid_bgp_topology_load_consistency ]------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_consistency($) {
  my ($cbgp)= @_;
  my $filename= get_resource("as-level-inconsistent.txt");
  (-e $filename) or return TEST_DISABLED;

  my $options= "--addr-sch=local --format=caida";

  my $error= cbgp_check_error($cbgp, "bgp topology load $options \"$filename\"");
  if (!defined($error) || !($error =~ m/topology is not consistent/)) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

