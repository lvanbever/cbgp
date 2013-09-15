return ["bgp topology load (loop)",
	"cbgp_valid_bgp_topology_load_loop"];

# -----[ cbgp_valid_bgp_topology_load_loop ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_loop($) {
  my ($cbgp)= @_;
  my $filename= get_tmp_resource("as-level-loop.txt");
  my $options= "--addr-sch=local --format=caida";

  die if !open(AS_LEVEL, ">$filename");
  print AS_LEVEL "123 123 1\n";
  close(AS_LEVEL);

  my $error= cbgp_check_error($cbgp, "bgp topology load $options \"$filename\"");
  if (!defined($error) || !($error =~ m/loop link/)) {
    return TEST_FAILURE;
  }

  unlink($filename);

  return TEST_SUCCESS;
}

