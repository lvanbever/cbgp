use strict;

return ["bgp decision process origin",
	"cbgp_valid_bgp_dp_origin"];

# -----[ cbgp_valid_bgp_dp_origin ]----------------------------------
# Test ability to break ties based on the ORIGIN attribute.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) virtual peer of R1
#   - R3 (1.0.0.3, AS3) virtual peer of R2
#
# Topology:
#
#   R2 --*
#         \
#          *-- R1
#         /
#   R3 --*
#
# Scenario:
#   * Advertise 255/8 to R1 from R2 with ORIGIN=EGP and R3 with
#     ORIGIN=IGP
#   * Check that R1 has received two routes
#   * Check that best route is through R3 (ORIGIN=IGP)
# -------------------------------------------------------------------
sub cbgp_valid_bgp_dp_origin($) {
  my ($cbgp)= @_;

  cbgp_topo_dp4($cbgp,
		["1.0.0.2", 1, 1, 1],
		["1.0.0.3", 1, 1, 1]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "255/8|2|EGP|1.0.0.2|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		   "255/8|2|IGP|1.0.0.3|0|0");

  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>"1.0.0.3"));

  return TEST_SUCCESS;
}

