use strict;

return ["bgp decision process igp",
	"cbgp_valid_bgp_dp_igp"];

# -----[ cbgp_valid_bgp_dp_igp ]-------------------------------------
sub cbgp_valid_bgp_dp_igp($) {
  my ($cbgp)= @_;
  cbgp_topo_dp3($cbgp, ["1.0.0.2", 1, 30], ["1.0.0.3", 1, 20],
		["1.0.0.4", 1, 10]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "255/16|2|IGP|1.0.0.2|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		   "255/16|2|IGP|1.0.0.3|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.4",
		   "255/16|2|IGP|1.0.0.4|0|0");
  $cbgp->send_cmd("sim run");
  my $rib;
  my @routes;
  $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
  @routes= values %$rib; 
  if (scalar(@routes) < 1) {
    return TEST_FAILURE;
  }
  if ($routes[0]->[F_RIB_NEXTHOP] ne "1.0.0.4") {
    return TEST_FAILURE;
  }
  cbgp_recv_withdraw($cbgp, "1.0.0.1", 1, "1.0.0.4", "255/16");
  $cbgp->send_cmd("sim run");
  $rib= cbgp_get_rib($cbgp, "1.0.0.1", "255/16");
  @routes= values %$rib; 
  if (scalar(@routes) < 1) {
    return TEST_FAILURE;
  }
  if ($routes[0]->[F_RIB_NEXTHOP] ne "1.0.0.3") {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

