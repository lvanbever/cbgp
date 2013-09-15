return ["bgp session ebgp", "cbgp_valid_bgp_session_ebgp"];

# -----[ cbgp_valid_bgp_session_ebgp ]-------------------------------
# Test eBGP redistribution mechanisms.
# - establishment
# - propagation
# - update AS-path on eBGP session
# - update next-hop on eBGP session
# - [TODO] local-pref is reset in routes received through eBGP
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS2)
#   - R3 (1.0.0.3, AS3)
#
# Topology:
#   R1 ---<eBGP>--- R2 ---<iBGP>--- R3
#
# Scenario:
#   * Originate 255.255/16 in R1, 255.254/16 in R3
#   * Check that R1 has 255.254/16 with next-hop=R2 and AS-Path="2"
#   * Check that R2 has 255.254/16 with next-hop=R3 and empty AS-Path
#   * Check that R2 has 255.255/16 with next-hop=R1 and AS-Path="1"
#   * Check that R3 has 255.255/16 with next-hop=R1 and AS-Path="1"
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_ebgp($) {
  my ($cbgp)= @_;

  my $topo= topo_3nodes_line();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp add router 2 1.0.0.2");
  $cbgp->send_cmd("bgp add router 2 1.0.0.3");
  cbgp_peering($cbgp, "1.0.0.1", "1.0.0.2", 2);
  cbgp_peering($cbgp, "1.0.0.2", "1.0.0.1", 1);
  cbgp_peering($cbgp, "1.0.0.2", "1.0.0.3", 2);
  cbgp_peering($cbgp, "1.0.0.3", "1.0.0.2", 2);
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_OPENWAIT) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_OPENWAIT));
  $cbgp->send_cmd("sim run");
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_ESTABLISHED) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_ESTABLISHED));
  $cbgp->send_cmd("bgp router 1.0.0.1 add network 255.255.0.0/16");
  $cbgp->send_cmd("bgp router 1.0.0.3 add network 255.254.0.0/16");
  $cbgp->send_cmd("sim run");

  # Check that AS-path contains remote ASN and that the next-hop is
  # the last router in remote AS
  my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.254/16",
			     -nexthop=>"1.0.0.2",
			     -path=>[2]));

  # Check that AS-path contains remote ASN
  $rib= cbgp_get_rib($cbgp, "1.0.0.2");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.254/16",
			     -nexthop=>"1.0.0.3",
			     -path=>undef));
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.255/16",
			     -nexthop=>"1.0.0.1",
			     -path=>[1]));

  # Check that AS-path contains remote ASN
  $rib= cbgp_get_rib($cbgp, "1.0.0.3");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255.255/16",
			     -nexthop=>"1.0.0.1",
			     -path=>[1]));

  return TEST_SUCCESS;
}

