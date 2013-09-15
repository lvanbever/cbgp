use strict;

return ["bgp decision process ebgp vs ibgp",
	"cbgp_valid_bgp_dp_ebgp_ibgp"];

# -----[ cbgp_valid_bgp_dp_ebgp_ibgp ]-------------------------------
# Test ability of the decision process to prefer routes received
# through eBGP sessions over routes received through iBGP sessions.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_dp_ebgp_ibgp($) {
  my ($cbgp)= @_;
  cbgp_topo_dp3($cbgp, ["1.0.0.2", 1, 0], ["2.0.0.1", 2, 0]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "255/16|2 5|IGP|1.0.0.2|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/16|2 5|IGP|2.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  my $rib;
  my @routes;
  $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
  @routes= values %$rib;
  if (scalar(@routes) < 1) {
    return TEST_FAILURE;
  }
  if ($routes[0]->[F_RIB_NEXTHOP] ne "2.0.0.1") {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

