return ["igp-bgp update med", "cbgp_valid_igp_bgp_med"];

# -----[ cbgp_valid_igp_bgp_med ]------------------------------------
# Test the ability to recompute IGP paths, reselect BGP routes and
# update MED accordingly.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) virtual peer of R1 (IGP cost:5)
#   - R3 (1.0.0.3, AS1) virtual peer of R1 (IGP cost:10)
#   - R4 (2.0.0.1, AS2) peer of R1 with "internal" MED advertisement
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
#   * Advertised 255/8 from R2 and R3
#   * Check default route selection: 255/8
#   * Check that MED value equals 5 (IGP weight)
#   * Increase IGP cost between R1 and R2, re-compute IGP, rescan
#   * Check that MED value equals 10 (IGP weight)
#   * Decrease IGP cost between R1 and R2, re-compute IGP, rescan
#   * Check that MED value equals 5 (IGP weight)
# -------------------------------------------------------------------
sub cbgp_valid_igp_bgp_med($) {
  my ($cbgp)= @_;

  cbgp_topo_igp_bgp($cbgp, ["1.0.0.2", 5], ["1.0.0.3", 10]);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "out",
	      "any", "metric internal");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "255/8|255|IGP|1.0.0.2|0|0|255:1");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		   "255/8|255|IGP|1.0.0.3|0|0|255:2");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>["255:1"],
			     -med=>5));

  # Increase weight of link R1-R2
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 100");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 100");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp domain 1 rescan");
  $cbgp->send_cmd("sim run");

  $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>["255:2"],
			     -med=>10));

  # Restore weight of link R1-R2
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 5");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 5");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp domain 1 rescan");
  $cbgp->send_cmd("sim run");

  $rib= cbgp_get_rib_mrt($cbgp, "2.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>["255:1"],
			     -med=>5));
  return TEST_SUCCESS;
}

