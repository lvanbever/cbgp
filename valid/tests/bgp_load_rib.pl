return ["bgp load rib", "cbgp_valid_bgp_load_rib"];

# -----[ cbgp_valid_bgp_load_rib ]-----------------------------------
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
sub cbgp_valid_bgp_load_rib($) {
  my ($cbgp)= @_;
  my $rib_file= get_resource("simple-rib.ascii");
  (-e $rib_file) or return TEST_DISABLED;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 2.0.0.1");
  $cbgp->send_cmd("net node 2.0.0.1 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1");
  $cbgp->send_cmd("net link 1.0.0.1 2.0.0.1 igp-weight --bidir 10");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("\tadd peer 2 2.0.0.1");
  $cbgp->send_cmd("\tpeer 2.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 2.0.0.1 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp router 1.0.0.1 load rib $rib_file");
  my $rib;
  $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  if (scalar(keys %$rib) != `cat $rib_file | wc -l`) {
    $tests->debug("number of prefixes mismatch");
    return TEST_FAILURE;
  }
  open(RIB, "<$rib_file") or die;
  while (<RIB>) {
    chomp;
    my @fields= split /\|/;
    $rib= cbgp_get_rib($cbgp, "1.0.0.1", $fields[MRT_PREFIX]);
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
