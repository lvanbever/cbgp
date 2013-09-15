return ["bgp load rib (full)", "cbgp_valid_bgp_load_rib_full"];

# -----[ cbgp_valid_bgp_load_rib_full ]------------------------------
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
sub cbgp_valid_bgp_load_rib_full($) {
  my ($cbgp)= @_;
  my $rib_file= get_resource("abilene-rib.ascii");
  (-e $rib_file) or return TEST_DISABLED;

  $cbgp->send_cmd("net add node 198.32.12.9");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.9");
  $cbgp->send_cmd("bgp router 198.32.12.9 load rib --autoconf $rib_file");
  my $rib;
  $rib= cbgp_get_rib($cbgp, "198.32.12.9");
  if (scalar(keys %$rib) != `cat $rib_file | wc -l`) {
    $tests->debug("number of prefixes mismatch");
    return TEST_FAILURE;
  }
  open(RIB, "<$rib_file") or die;
  while (<RIB>) {
    chomp;
    my @fields= split /\|/;
    $rib= cbgp_get_rib($cbgp, "198.32.12.9", $fields[MRT_PREFIX]);
    if (scalar(keys %$rib) != 1) {
      print "no route\n";
      return TEST_FAILURE;
    }
    my $canonic= canonic_prefix($fields[MRT_PREFIX]);
    if (!exists($rib->{$canonic})) {
      print "could not find route towards prefix $fields[MRT_PREFIX]\n";
      return TEST_FAILURE;
    }
    if ($fields[MRT_NEXTHOP] ne
	$rib->{$canonic}->[F_RIB_NEXTHOP]) {
      print "incorrect next-hop for $fields[MRT_PREFIX]\n";
      return TEST_FAILURE;
    }
  }
  close(RIB);
  return TEST_SUCCESS;
}

