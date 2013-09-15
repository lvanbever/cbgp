return ["nasty: bgp peering one-way down",
	"cbgp_valid_nasty_bgp_peering_1w_down"];

# -----[ cbgp_valid_nasty_bgp_peering_1w_down ]----------------------
# Test that C-BGP detects that a session is down even when one
# direction still works.
#
# Setup:
#   - R1 (0.1.0.0, AS1)
#   - R2 (0.2.0.0, AS2)
#
# Topology:
#
#       eBGP
#   R1 ------ R2
#
# Scenario:
#   * Establish the BGP session
#   * Remove the route from R1 to R2
#   * Refresh BGP session state
#   * Check that the session is in ACTIVE state on both ends
# -------------------------------------------------------------------
sub cbgp_valid_nasty_bgp_peering_1w_down($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 0.1.0.0");
  $cbgp->send_cmd("net add node 0.2.0.0");
  $cbgp->send_cmd("net add link 0.1.0.0 0.2.0.0");
  $cbgp->send_cmd("net node 0.1.0.0 route add --oif=0.2.0.0 0.2/16 10");
  $cbgp->send_cmd("net node 0.2.0.0 route add --oif=0.1.0.0 0.1/16 10");

  $cbgp->send_cmd("bgp add router 1 0.1.0.0");
  $cbgp->send_cmd("bgp router 0.1.0.0");
  $cbgp->send_cmd("\tadd peer 2 0.2.0.0");
  $cbgp->send_cmd("\tpeer 0.2.0.0 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 2 0.2.0.0");
  $cbgp->send_cmd("bgp router 0.2.0.0");
  $cbgp->send_cmd("\tadd peer 1 0.1.0.0");
  $cbgp->send_cmd("\tpeer 0.1.0.0 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("sim run");

  my $peers= cbgp_get_peers($cbgp, "0.1.0.0");
  return TEST_FAILURE
    if (!check_has_peer($peers, "0.2.0.0",
			-state=>C_PEER_ESTABLISHED));
  $peers= cbgp_get_peers($cbgp, "0.2.0.0");
  return TEST_FAILURE
    if (!check_has_peer($peers, "0.1.0.0",
			-state=>C_PEER_ESTABLISHED));

  $cbgp->send_cmd("net node 0.1.0.0 route del 0.2/16 *");
  $cbgp->send_cmd("bgp router 0.1.0.0 rescan");
  $cbgp->send_cmd("bgp router 0.2.0.0 rescan");

  $peers= cbgp_get_peers($cbgp, "0.1.0.0");
  return TEST_FAILURE
    if (!check_has_peer($peers, "0.2.0.0",
			-state=>C_PEER_ACTIVE));
  $peers= cbgp_get_peers($cbgp, "0.2.0.0");
  return TEST_FAILURE
    if (!check_has_peer($peers, "0.1.0.0",
			-state=>C_PEER_ACTIVE));
  return TEST_SUCCESS;
}


