return ["bgp topology load (format=meulle)",
	"cbgp_valid_bgp_topology_load_meulle"];

# -----[ cbgp_valid_bgp_topology_load_meulle ]-----------------------
# Test the ability to load AS-level topologies provided by Michael
# Meulle (France Telecom R&D). The format is as follows. Each line
# is a series of 3 fields separated by tabulations:
#   AS<i> AS<j> <relationship>
# where <i> and <j> are the AS numbers and <relationship> is one of
# P2C, PEER and C2P.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load_meulle($) {
  my ($cbgp)= @_;
  my $filename= get_tmp_resource("as-level-meulle.txt");
  my $options= "--format=meulle";
  my $error;

  die if !open(AS_LEVEL, ">$filename");
  print AS_LEVEL "AS1\tAS2\tP2C\n";
  print AS_LEVEL "AS2\tAS3\tPEER\n";
  print AS_LEVEL "AS2\tAS1\tC2P\n";
  print AS_LEVEL "AS3\tAS2\tPEER\n";
  close(AS_LEVEL);

  $error= cbgp_check_error($cbgp,"bgp topology load $options \"$filename\"");
  defined($error) and return TEST_FAILURE;
  $error= cbgp_check_error($cbgp, "bgp topology install");
  defined($error) and return TEST_FAILURE;

  return TEST_SUCCESS;
}

