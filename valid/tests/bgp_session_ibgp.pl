return ["bgp session ibgp", "cbgp_valid_bgp_session_ibgp"];

# -----[ cbgp_valid_bgp_session_ibgp ]-------------------------------
# Test iBGP redistribution mechanisms.
# - establishment
# - propagation
# - no iBGP reflection
# - no AS-path update
# - no next-hop update
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1
#
# Topology:
#   R1 ----- R2 ----- R3
#
# Scenario:
#   * Originate 255.255/16 in R1
#   * Check that R2 receives the route (with next-hop=R1 and empty
#     AS-Path)
#   * Check that R3 does not receive the route (no iBGP reflection)
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_ibgp($) {
  my ($cbgp)= @_;
  my $topo= topo_3nodes_line();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  cbgp_topo_bgp_routers($cbgp, $topo, 1);
  cbgp_peering($cbgp, "1.0.0.1", "1.0.0.2", 1);
  cbgp_peering($cbgp, "1.0.0.2", "1.0.0.1", 1);
  cbgp_peering($cbgp, "1.0.0.2", "1.0.0.3", 1);
  cbgp_peering($cbgp, "1.0.0.3", "1.0.0.2", 1);
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_OPENWAIT) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_OPENWAIT) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			 -state=>C_PEER_OPENWAIT) ||
	!cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			 -state=>C_PEER_OPENWAIT));
  $cbgp->send_cmd("sim run");
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_ESTABLISHED) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_ESTABLISHED) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			 -state=>C_PEER_ESTABLISHED) ||
	!cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			 -state=>C_PEER_ESTABLISHED));
  $cbgp->send_cmd("bgp router 1.0.0.1 add network 255.255.0.0/16");
  $cbgp->send_cmd("sim run");

  my $rib;
  # Check that 1.0.0.2 has received the route, that the next-hop is
  # 1.0.0.1 and the AS-path is empty. 
  $rib= cbgp_get_rib($cbgp, "1.0.0.2");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.255/16",
			     -nexthop=>"1.0.0.1",
			     -path=>undef));

  # Check that 1.0.0.3 has NOT received the route (1.0.0.2 did not
  # send it since it was learned through an iBGP session).
  $rib= cbgp_get_rib($cbgp, "1.0.0.3");
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255.255/16"));

  return TEST_SUCCESS;
}

