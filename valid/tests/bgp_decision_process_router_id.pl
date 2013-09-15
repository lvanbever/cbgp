use strict;

return ["bgp decision process router-id",
	"cbgp_valid_bgp_dp_router_id"];

# -----[ cbgp_valid_bgp_dp_router_id ]-------------------------------
sub cbgp_valid_bgp_dp_router_id($) {
  my ($cbgp)= @_;
  cbgp_topo_dp($cbgp, 2);
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
  if ($routes[0]->[F_RIB_NEXTHOP] ne "2.0.0.1") {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

