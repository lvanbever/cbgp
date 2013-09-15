return ["bgp soft-restart", "cbgp_valid_bgp_soft_restart"];

# -----[ cbgp_valid_bgp_soft_restart ]-------------------------------
# Test ability to maintain the content of the Adj-RIB-in of a virtual
# peer when the session is down (or broken).
# -------------------------------------------------------------------
sub cbgp_valid_bgp_soft_restart($) {
  my ($cbgp)= @_;
  my $topo= topo_2nodes();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  cbgp_topo_bgp_routers($cbgp, $topo, 1);
  $cbgp->send_cmd("bgp domain 1 full-mesh");
  $cbgp->send_cmd("net add node 2.0.0.1");
  $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 route add --oif=2.0.0.1 2.0.0.1/32 0");
  cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2,
	       "next-hop-self", "virtual", "soft-restart");
  $cbgp->send_cmd("sim run");
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "2.0.0.1",
			 -state=>C_PEER_ESTABLISHED));
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
		  "\"BGP4|0|A|1.0.0.1|1|255.255.0.0/16|2|IGP|2.0.0.1|0|0\"");
  $cbgp->send_cmd("sim run");

  my $rib;

  # Check that route is available
  $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.255/16"));

  $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 down");
  $cbgp->send_cmd("sim run");

  # Check that route has been withdrawn
  $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255.255/16"));

  $cbgp->send_cmd("bgp router 1.0.0.1 peer 2.0.0.1 up");
  $cbgp->send_cmd("sim run");
  # Check that route is still announced after peering up
  $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.255/16"));

  return TEST_SUCCESS;
}

