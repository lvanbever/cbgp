return ["bgp topology policies (filters-caida)",
	"cbgp_valid_bgp_topology_policies_filters_caida"];

# -----[ cbgp_valid_bgp_topology_policies_filters_caida ]------------
# Test that routing filters are setup correctly by the "bgp topology
# policies" statement, in case of the CAIDA format, i.e. including
# sibling-to-sibling relationships.
#
# Setup:
#   - AS1 (0.1.0.0)
#   - AS2 (0.2.0.0) provider of AS1
#   - AS3 (0.3.0.0) provider of AS1
#   - AS4 (0.4.0.0) peer of AS1
#   - AS5 (0.5.0.0) peer of AS1
#   - AS6 (0.6.0.0) customer of AS1
#   - AS7 (0.7.0.0) customer of AS1
#   - AS8 (0.8.0.0) sibling of AS1
#   - AS9 (0.9.0.0) sibling of AS1
#
# Topology:
#
#   AS2 ----*     *----- AS3
#            \   /
#   AS4 ----* \ / *----- AS5
#            \| |/
#             AS1
#            /| |\
#   AS8 ----* / \ *----- AS9
#            /   \
#   AS6 ----*     *----- AS7
#
# Scenario:
#   * Originate 255/8 from AS6, 254/8 from AS4, 253/8 from AS2,
#     252/8 from AS1, 240/8 from AS8
#   * Check that AS1 has all routes
#   * Check that AS7 has all routes
#   * Check that AS5 has only AS1, AS6 and AS8
#   * Check that AS3 has only AS1, AS6 and AS8
#   * Originate 251/8 from AS3, AS5, AS7, check that AS1 selects AS7
#   * Originate 250/8 from AS3, AS5, check that AS1 selects AS5
#   * Originate 249/8 from AS5, AS7, check that AS1 selects AS7
#   * Originate 248/8 from AS3, AS7, check that AS1 selects AS7
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_policies_filters_caida($) {
  my ($cbgp)= @_;
  my $filename= get_tmp_resource("as-level-policies-filter.topo");

  # Create CAIDA AS-level file
  open(AS_LEVEL_TOPO, ">$filename") or die;
  print AS_LEVEL_TOPO "2 1 1\n";
  print AS_LEVEL_TOPO "1 2 -1\n";
  print AS_LEVEL_TOPO "3 1 1\n";
  print AS_LEVEL_TOPO "1 3 -1\n";
  print AS_LEVEL_TOPO "1 4 0\n";
  print AS_LEVEL_TOPO "4 1 0\n";
  print AS_LEVEL_TOPO "1 6 1\n";
  print AS_LEVEL_TOPO "6 1 -1\n";
  print AS_LEVEL_TOPO "1 5 0\n";
  print AS_LEVEL_TOPO "5 1 0\n";
  print AS_LEVEL_TOPO "1 7 1\n";
  print AS_LEVEL_TOPO "7 1 -1\n";
  print AS_LEVEL_TOPO "8 1 2\n";
  print AS_LEVEL_TOPO "1 8 2\n";
  print AS_LEVEL_TOPO "9 1 2\n";
  print AS_LEVEL_TOPO "1 9 2\n";
  close(AS_LEVEL_TOPO);

  # Load topology and setup routing filters
  $cbgp->send_cmd("bgp topology load --format=caida \"$filename\"");
  $cbgp->send_cmd("bgp topology install");
  $cbgp->send_cmd("bgp topology policies");
  $cbgp->send_cmd("bgp topology run");
  $cbgp->send_cmd("sim run");

  # Originate prefixes and propagate
  $cbgp->send_cmd("bgp router 0.1.0.0 add network 252/8");
  $cbgp->send_cmd("bgp router 0.2.0.0 add network 253/8");
  $cbgp->send_cmd("bgp router 0.4.0.0 add network 254/8");
  $cbgp->send_cmd("bgp router 0.6.0.0 add network 255/8");
  $cbgp->send_cmd("sim run");

  # Check valley-free property for 252-255/8
  my $rib= cbgp_get_rib($cbgp, "0.1.0.0");
  if (!exists($rib->{"252.0.0.0/8"}) || !exists($rib->{"253.0.0.0/8"}) ||
      !exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
    $tests->debug("AS1 should have all routes");
    return TEST_FAILURE;
  }
  $rib= cbgp_get_rib($cbgp, "0.3.0.0");
  if (!exists($rib->{"252.0.0.0/8"}) || exists($rib->{"253.0.0.0/8"}) ||
      exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
    $tests->debug("AS3 should only receive route from AS1 and AS6");
    return TEST_FAILURE;
  }
  $rib= cbgp_get_rib($cbgp, "0.5.0.0");
  if (!exists($rib->{"252.0.0.0/8"}) || exists($rib->{"253.0.0.0/8"}) ||
      exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
    $tests->debug("AS5 should only receive route from AS1 and AS6");
    return TEST_FAILURE;
  }
  $rib= cbgp_get_rib($cbgp, "0.7.0.0");
  if (!exists($rib->{"252.0.0.0/8"}) || !exists($rib->{"253.0.0.0/8"}) ||
      !exists($rib->{"254.0.0.0/8"}) || !exists($rib->{"255.0.0.0/8"})) {
    $tests->debug("AS7 should receive all routes");
    return TEST_FAILURE;
  }

  # Originate prefixes and propagate
  $cbgp->send_cmd("bgp router 0.3.0.0 add network 251/8");
  $cbgp->send_cmd("bgp router 0.5.0.0 add network 251/8");
  $cbgp->send_cmd("bgp router 0.7.0.0 add network 251/8");
  $cbgp->send_cmd("bgp router 0.3.0.0 add network 250/8");
  $cbgp->send_cmd("bgp router 0.5.0.0 add network 250/8");
  $cbgp->send_cmd("bgp router 0.5.0.0 add network 249/8");
  $cbgp->send_cmd("bgp router 0.7.0.0 add network 249/8");
  $cbgp->send_cmd("bgp router 0.3.0.0 add network 248/8");
  $cbgp->send_cmd("bgp router 0.7.0.0 add network 248/8");
  $cbgp->send_cmd("sim run");

  # Check preferences for 248-251/8
  $rib= cbgp_get_rib($cbgp, "0.1.0.0");
  if (!exists($rib->{"248.0.0.0/8"}) || !exists($rib->{"249.0.0.0/8"}) ||
      !exists($rib->{"250.0.0.0/8"}) || !exists($rib->{"251.0.0.0/8"})) {
    $tests->debug("AS1 should receive all routes");
    return TEST_FAILURE;
  }
  if (($rib->{"248.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.7.0.0') ||
      ($rib->{"249.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.7.0.0') ||
      ($rib->{"250.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.5.0.0') ||
      ($rib->{"251.0.0.0/8"}->[F_RIB_NEXTHOP] ne '0.7.0.0')) {
    $tests->debug("AS1's route selection is incorrect");
    return TEST_FAILURE;
  }

  # Remove temporary file
  unlink($filename);

  return TEST_SUCCESS;
}
