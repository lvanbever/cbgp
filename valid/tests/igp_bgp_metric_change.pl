return ["igp-bgp metric change", "cbgp_valid_igp_bgp_metric_change"];

# -----[ cbgp_valid_igp_bgp_metric_change ]--------------------------
# Test the ability to recompute IGP paths and reselect BGP routes
# accordingly.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) virtual peer of R1 (IGP cost:5)
#   - R3 (1.0.0.3, AS1) virtual peer of R1 (IGP cost:10)
#   - R4 (2.0.0.1, AS2) peer of R1
#
# Topology:
#
#   (R2) --*
#           \
#            R1 ----- R4
#           /
#   (R3) --*
#
# Scenario:
#   * Advertised 255/8 [255:1] from R2, 255/8 [255:2] from R3
#   * Check default route selection: 255/8 [255:1]
#   * Increase IGP cost between R1 and R2, re-compute IGP, rescan.
#   * Check that route is now 255/8 [255:2]
#   * Decrease IGP cost between R1 and R2, re-compute IGP, rescan
#   * Check that default route is selected
# -------------------------------------------------------------------
sub cbgp_valid_igp_bgp_metric_change() {
  my ($cbgp)= @_;

  cbgp_topo_igp_bgp($cbgp, ["1.0.0.2", 5], ["1.0.0.3", 10]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "255/8|255|IGP|1.0.0.2|0|0|255:1");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		   "255/8|255|IGP|1.0.0.3|0|0|255:2");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>["255:1"]));

  # Increase weight of link R1-R2
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 100");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 100");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp domain 1 rescan");
  $cbgp->send_cmd("sim run");

  $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>["255:2"]));

  # Restore weight of link R1-R2
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 5");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 5");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp domain 1 rescan");
  $cbgp->send_cmd("sim run");

  $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>["255:1"]));
  return TEST_SUCCESS;
}

