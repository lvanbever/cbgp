use strict;

return ["bgp decision process local-pref",
	"cbgp_valid_bgp_dp_localpref"];

# -----[ cbgp_valid_bgp_dp_localpref ]-------------------------------
# Test ability of decision-process to select route with the highest
# value of the local-pref attribute.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_dp_localpref($) {
  my ($cbgp)= @_;
  cbgp_topo_dp($cbgp, 2);
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "local-pref 100");
  cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in", "any", "local-pref 80");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/16|2 4|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "255/16|3 4|IGP|3.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  my $rib;
  my @routes;
  $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
  @routes= values %$rib; 
  if (scalar(@routes) < 1) {
    return TEST_FAILURE;
  }
  if ($routes[0]->[F_RIB_PREF] != 100) {
    return TEST_FAILURE;
  }
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 down");
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 3.0.0.1 down");
  $cbgp->send_cmd("sim run");
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 filter in remove-rule 0");
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 3.0.0.1 filter in remove-rule 0");
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 up");
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 3.0.0.1 up");
  $cbgp->send_cmd("sim run");
  cbgp_filter($cbgp, "1.0.0.1", "2.0.0.1", "in", "any", "local-pref 80");
  cbgp_filter($cbgp, "1.0.0.1", "3.0.0.1", "in", "any", "local-pref 100");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/16|2|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "255/16|2|IGP|3.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
  @routes= values %$rib; 
  if (scalar(@routes) < 1) {
    return TEST_FAILURE;
  }
  if ($routes[0]->[F_RIB_PREF] != 100) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

