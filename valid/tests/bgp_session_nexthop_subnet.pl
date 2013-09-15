return ["bgp session nexthop subnet",
	"cbgp_valid_bgp_session_nexthop_subnet"];

# -----[ cbgp_valid_bgp_session_nexthop_subnet ]---------------------
# Check that BGP can install a route with a next-hop reachable
# through a subnet.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS2)
#   - R3 (2.0.0.1, AS2, virtual)
#   - N1 (192.168.0/24)
#   - R1.N1 = 192.168.0.1
#   - R3.N1 = 192.168.0.2
#
# Topology:
#
#   R3 ---{N1}--- R1 ----- R2
#      .2      .1
#
# Scenario:
#   * Advertise a BGP route from R3 to R1, run the simulation
#   * Check that R1 has installed the route with nh:R3.N1 and if:R1->N1
#   * Check that R2 has installed the route with nh:0.0.0.0 and if:R2->R1
# -------------------------------------------------------------------
sub cbgp_valid_bgp_session_nexthop_subnet($) {
  my ($cbgp)= @_;
  my $rt;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add domain 2 igp");
  $cbgp->send_cmd("net add node 2.0.0.1");
  $cbgp->send_cmd("net node 2.0.0.1 domain 2");
  $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24");
  $cbgp->send_cmd("net link 1.0.0.1 192.168.0.1/24 igp-weight 10");
  $cbgp->send_cmd("net add link 2.0.0.1 192.168.0.2/24");
  $cbgp->send_cmd("net link 2.0.0.1 192.168.0.2/24 igp-weight 10");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("net domain 2 compute");
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\tadd peer 2 192.168.0.2");
  $cbgp->send_cmd("\tpeer 192.168.0.2 virtual");
  $cbgp->send_cmd("\tpeer 192.168.0.2 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp router 1.0.0.1 peer 192.168.0.2 recv \"BGP4|0|A|1.0.0.1|1|255/8|2 3|IGP|192.168.0.2|12|34|\"");
  $cbgp->send_cmd("sim run");

  return TEST_FAILURE
    if (!cbgp_check_route($cbgp, '1.0.0.2', '255.0.0.0/8',
			  -iface=>'---',
			  -nexthop=>'192.168.0.2'));
  return TEST_FAILURE
    if (!cbgp_check_route($cbgp, '1.0.0.1', '255.0.0.0/8',
			  -iface=>'---',
			  -nexthop=>'192.168.0.2'));

  my $tr= cbgp_record_route($cbgp, '1.0.0.2', '255.0.0.0');
  return TEST_FAILURE
    if (!check_recordroute($tr, -status=>"UNREACH",
			   -path=>[[1,'1.0.0.1'],
				   [2,'2.0.0.1']]));
  return TEST_SUCCESS;
}

