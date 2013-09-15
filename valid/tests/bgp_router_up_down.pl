return ["bgp router up/down", "cbgp_valid_bgp_router_up_down"];

# -----[ cbgp_valid_bgp_router_up_down ]-----------------------------
# Test ability to start and stop a router. Starting (stopping) a
# router should be equivalent to establishing (shutting down) all its
# BGP sessions.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#
# Topology:
#   R1 ----- R2 ----- R3
#
# Scenario:
#   * setup topology, routers and sessions
#   * check that all sessions are in state IDLE
#   * start all BGP routers
#   * check that all sessions are in state OPENWAIT
#   * run the simulation
#   * check that all sessions are in state ESTABLISHED
#   * stop all BGP routers, run the simulation
#   * check that all sessions are in state IDLE
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_up_down($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("\tadd network 255/8");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 1 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("\tadd network 255/8");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\texit");

  # All sessions should be in IDLE state
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_IDLE) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_IDLE) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			 -state=>C_PEER_IDLE) ||
	!cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			 -state=>C_PEER_IDLE));

  $cbgp->send_cmd("bgp router 1.0.0.1 start");
  $cbgp->send_cmd("bgp router 1.0.0.2 start");
  $cbgp->send_cmd("bgp router 1.0.0.3 start");
  # All sessions should be in OPENWAIT state
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
  # All sessions should be in ESTABLISHED state
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_ESTABLISHED) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_ESTABLISHED) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			 -state=>C_PEER_ESTABLISHED) ||
	!cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			 -state=>C_PEER_ESTABLISHED));

  $cbgp->send_cmd("bgp router 1.0.0.1 stop");
  $cbgp->send_cmd("bgp router 1.0.0.2 stop");
  $cbgp->send_cmd("bgp router 1.0.0.3 stop");
  $cbgp->send_cmd("sim run");
  # All sessions should be in IDLE state
  return TEST_FAILURE
    if (!cbgp_check_peer($cbgp, "1.0.0.1", "1.0.0.2",
			 -state=>C_PEER_IDLE) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.1",
			 -state=>C_PEER_IDLE) ||
	!cbgp_check_peer($cbgp, "1.0.0.2", "1.0.0.3",
			 -state=>C_PEER_IDLE) ||
	!cbgp_check_peer($cbgp, "1.0.0.3", "1.0.0.2",
			 -state=>C_PEER_IDLE));

  return TEST_SUCCESS;
}

