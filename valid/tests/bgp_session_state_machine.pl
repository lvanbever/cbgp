return ["bgp session state-machine", "cbgp_valid_bgp_session_sm"];

# -----[ cbgp_valid_bgp_session_sm ]---------------------------------
# This test validate the BGP session state-machine. This is not an
# exhaustive test, however it covers most of the situations.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#
# Topology:
#   R1 ----- R2
#
# Scenario:
#   * create the topology, but do not compute the routes between
#     R1 and R2, the BGP session is configured but not running
#   * check that both peerings are in state IDLE
#   * set peering from R1 to R2 as running
#   * check that R1 -> R2 is in state ACTIVE (since route does not
#     exist)
#   * compute routes between R1 and R2 and rerun peering R1 -> R2
#   * check that R1 -> R2 is in state OPENWAIT
#   * set peering R2 -> R2 as running
#   * check that R2 -> R1 is in state OPENWAIT
#   * run the simulation (OPEN messages should be exchanged)
#   * check that both peerings are in state ESTABLISHED
#   * close peering R1 -> R2
#   * check that R1 -> R2 is in state IDLE
#   * run the simulation
#   * check that R2 -> R1 is in state ACTIVE
#   * re-run peering R1 -> R2 and run the simulation
#   * check that both peerings are in state ESTABLISHED
#   * close peering R1 -> R2, run the simulation, then close R2 -> R1
#   * check that peering R2 -> R1 is in state IDLE
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_sm() {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1 add peer 1 1.0.0.2");
  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2 add peer 1 1.0.0.1");

  # At this point, both peerings should be in state IDLE
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_IDLE) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_IDLE));

  $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 up");
  # At this point, peering R1 -> R2 should be in state ACTIVE
  # because there is no route from R1 to R2
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_ACTIVE));

  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp router 1.0.0.1 rescan");
  # At this point, peering R1 -> R2 should be in state OPENWAIT
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_OPENWAIT));

  $cbgp->send_cmd("bgp router 1.0.0.2 peer 1.0.0.1 up");
  # At this point, peering R2 -> R1 should also be in state OPENWAIT
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_OPENWAIT));

  $cbgp->send_cmd("sim run");
  # At this point, both peerings should be in state ESTABLISHED
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_ESTABLISHED) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_ESTABLISHED));

  $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 down");
  # At this point, peering R1 -> R2 should be in state IDLE
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_IDLE));

  $cbgp->send_cmd("sim run");
  # At this point, peering R2 -> R1 should be in state ACTIVE
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_ACTIVE));

  $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 up");
  $cbgp->send_cmd("sim run");
  # At this point, both peerings should be in state ESTABLISHED
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_ESTABLISHED) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_ESTABLISHED));

  $cbgp->send_cmd("bgp router 1.0.0.1 peer 1.0.0.2 down");
  $cbgp->send_cmd("sim run");
  $cbgp->send_cmd("bgp router 1.0.0.2 peer 1.0.0.1 down");
  # At this point, peering R2 -> R1 should be in state IDLE
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_IDLE));

  return TEST_SUCCESS;
}

