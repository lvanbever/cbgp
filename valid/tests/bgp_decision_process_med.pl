use strict;

return ["bgp decision process med",
	"cbgp_valid_bgp_dp_med"];

# -----[ cbgp_valid_bgp_dp_med ]-------------------------------------
# Test ability of the decision process to prefer routes with the
# highest MED value.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2)
#   - R3 (2.0.0.2, AS2)
#   - R4 (3.0.0.1, AS3)
#   - R5 (3.0.0.2, AS3)
#
# Topology:
#
#   (R2)---*
#           \
#   (R3)---* \
#           \ \
#             R1
#           / /
#   (R4)---* /
#           /
#   (R5)---*
#
# Scenario:
#   * R2 announces 255.0/16 with MED=80
#     R3 announces 255.0/16 with MED=60
#     R4 announces 255.0/16 with MED=40
#     R5 announces 255.0/16 with MED=20
#   * Check that best route selected by R1 is the route announced
#     by R3 (2.0.0.2)
# -------------------------------------------------------------------
sub cbgp_valid_bgp_dp_med($) {
  my ($cbgp)= @_;
  cbgp_topo_dp3($cbgp, ["2.0.0.1", 2, 0], ["2.0.0.2", 2, 0],
		["3.0.0.1", 3, 0], ["3.0.0.2", 3, 0]);
  $cbgp->send_cmd("bgp options med deterministic");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/16|2 4|IGP|2.0.0.1|0|80");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.2",
		   "255/16|2 4|IGP|2.0.0.2|0|60");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "255/16|3 4|IGP|3.0.0.1|0|40");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.2",
		   "255/16|3 4|IGP|3.0.0.2|0|20");
  $cbgp->send_cmd("sim run");
  my $rib;
  my @routes;
  $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
  @routes= values %$rib; 
  if (scalar(@routes) < 1) {
    return TEST_FAILURE;
  }
  if ($routes[0]->[F_RIB_NEXTHOP] ne "2.0.0.2") {
    return TEST_FAILURE;
  }
  $cbgp->send_cmd("bgp options med always-compare");
  cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "2.0.0.1", "255/16");
  cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "2.0.0.2", "255/16");
  cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "3.0.0.1", "255/16");
  cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "3.0.0.2", "255/16");
  $cbgp->send_cmd("sim run");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/16|2 4|IGP|2.0.0.1|0|80");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.2",
		   "255/16|2 4|IGP|2.0.0.2|0|60");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "255/16|3 4|IGP|3.0.0.1|0|40");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.2",
		   "255/16|3 4|IGP|3.0.0.2|0|20");
  $cbgp->send_cmd("sim run");
  $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
  @routes= values %$rib; 
  if (scalar(@routes) < 1) {
    return TEST_FAILURE;
  }
  if ($routes[0]->[F_RIB_NEXTHOP] ne "3.0.0.2") {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

