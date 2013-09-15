return ["bgp topology load (duplicate)",
	"cbgp_valid_bgp_topology_load_duplicate"];

# -----[ cbgp_valid_bgp_topology_load_duplicate ]--------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_duplicate($) {
  my ($cbgp)= @_;
  my $filename= get_tmp_resource("as-level-duplicate.txt");
  my $options= "--addr-sch=local --format=caida";

  die if !open(AS_LEVEL, ">$filename");
  print AS_LEVEL "1 2 -1\n1 2 0\n";
  close(AS_LEVEL);

  my $error= cbgp_check_error($cbgp, "bgp topology load $options \"$filename\"");
  if (!defined($error) || !($error =~ m/duplicate link/)) {
    return TEST_FAILURE;
  }

  unlink($filename);

  return TEST_SUCCESS;
}

