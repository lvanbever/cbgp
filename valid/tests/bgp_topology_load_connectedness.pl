return ["bgp topology check (connectedness)", "cbgp_valid_bgp_topology_check_connectedness"];

# -----[ cbgp_valid_bgp_topology_check_connectedness ]---------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_check_connectedness($) {
  my ($cbgp)= @_;
  my $filename= get_resource("as-level-disconnected.txt");
  (-e $filename) or return TEST_DISABLED;

  my $options= "--addr-sch=local";

  $cbgp->send_cmd("bgp topology load $options \"$filename\"");
  cbgp_checkpoint($cbgp);
  my $error= cbgp_check_error($cbgp, "bgp topology check");
  if (!defined($error) || !($error =~ m/topology is not connected/)) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

