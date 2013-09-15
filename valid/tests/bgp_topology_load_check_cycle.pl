return ["bgp topology check (cycle)", "cbgp_valid_bgp_topology_check_cycle"];

# -----[ cbgp_valid_bgp_topology_check_cycle ]-----------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_check_cycle($) {
  my ($cbgp)= @_;
  my $filename= get_resource("as-level-cycle.txt");
  (-e $filename) or return TEST_DISABLED;

  my $options= "--addr-sch=local";

  $cbgp->send_cmd("bgp topology load $options \"$filename\"");
  cbgp_checkpoint($cbgp);
  my $error= cbgp_check_error($cbgp, "bgp topology check");
  if (!defined($error) || !($error =~ m/topology contains cycle\(s\)/)) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

