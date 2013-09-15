return ["bgp recv mrt", "cbgp_valid_bgp_recv_mrt"];

# -----[ cbgp_valid_bgp_recv_mrt ]-----------------------------------
# Test ability to inject BGP routes in the model through virtual
# peers. BGP routes are specified in the MRT format.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (2.0.0.1, AS2), virtual
#
# Topology:
#
#   R2 ----- R1 ----- (R3)
#
# Scenario:
#   * Inject route towards 255.255/16 from R3 to R1
#   * Check that R1 has received the route with correct attributes
#   * Send a withdraw for the same prefix from R3 to R1
#   * Check that R1 has not the route anymore
# -------------------------------------------------------------------
sub cbgp_valid_bgp_recv_mrt($) {
  my ($cbgp)= @_;
  my $topo= topo_2nodes();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  cbgp_topo_bgp_routers($cbgp, $topo, 1);
  $cbgp->send_cmd("bgp domain 1 full-mesh");
  $cbgp->send_cmd("net add node 2.0.0.1");
  $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 route add --oif=2.0.0.1 2.0.0.1/32 0");
  cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self", "virtual");
  $cbgp->send_cmd("sim run");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255.255/16|2|IGP|2.0.0.1|12|34");
  $cbgp->send_cmd("sim run");

  my $rib;
  # Check attributes of received route (note: local-pref has been
  # reset since the route is received over an eBGP session).
  $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.255/16",
			     -nexthop=>"2.0.0.1",
			     -pref=>0,
			     -med=>34,
			     -path=>[2]));

  $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		  "\"BGP4|0|W|1.0.0.1|1|255.255.0.0/16\"");
  $cbgp->send_cmd("sim run");

  # Check that the route has been withdrawn
  $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255.255/16"));

  return TEST_SUCCESS;
}

