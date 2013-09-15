return ["bgp load rib (full,binary)", "cbgp_valid_bgp_load_rib_full_binary"];

# -----[ cbgp_valid_bgp_load_rib_full_binary ]-----------------------
# Test ability to load a BGP dump into a router. The content of the
# BGP dump must be specified in MRT format.
#
# Setup:
#   - R1 (198.32.12.9, AS11537)
#
# Scenario:
#   * Load BGP dump collected in Abilene into router
#   * Check that all routes are loaded in the router with the right
#     attributes
#
# Resources:
#   [abilene-rib.ascii]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_load_rib_full_binary($) {
  my ($cbgp)= @_;
  my $rib_file= get_resource("abilene-rib.gz");
  my $rib_file_ascii= get_resource("abilene-rib.marco");

  ((-e $rib_file) and (-e $rib_file_ascii))
    or return TEST_DISABLED;

  $cbgp->send_cmd("net add node 198.32.12.9");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.9");
  $cbgp->send_cmd("bgp router 198.32.12.9 load rib --autoconf --format=mrt-binary $rib_file");

  # Use Marco d'Itri's zebra-dump-parser to compare content
  # File obtained using
  #   zcat abilene-rib.gz | ./zebra-dump-parser.pl > abilene-rib.marco
  my $num_routes= 0;
  open(ZEBRA_DUMP_PARSER, "$rib_file_ascii") or die;
  while (<ZEBRA_DUMP_PARSER>) {
    chomp;
    my @fields= split /\s+/, $_, 2;
    $num_routes++;
    my $rib= cbgp_get_rib($cbgp, "198.32.12.9", $fields[0]);
    if (!defined($rib) || !exists($rib->{$fields[0]})) {
      return TEST_FAILURE;
    }
  }
  close(ZEBRA_DUMP_PARSER);

  # Check that number of routes reported by Marco's script is
  # equal to number of routes in RIB.
  my $rib;
  $rib= cbgp_get_rib($cbgp, "198.32.12.9");
  if (scalar(keys %$rib) != $num_routes) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

