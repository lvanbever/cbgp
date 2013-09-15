use strict;

return ["bgp decision process as-path",
	"cbgp_valid_bgp_dp_aspath"];

# -----[ cbgp_valid_bgp_dp_aspath ]----------------------------------
# Test ability of decision process to select route with the shortest
# AS-Path.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_dp_aspath($) {
  my ($cbgp)= @_;
  cbgp_topo_dp($cbgp, 2);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/16|2 4 5|IGP|2.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "255/16|3 5|IGP|3.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  my $rib;
  my @routes;
  $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
  @routes= values %$rib; 
  if (scalar(@routes) < 1) {
    return TEST_FAILURE;
  }
  if (!aspath_equals($routes[0]->[F_RIB_PATH], [3, 5])) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

