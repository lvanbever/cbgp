return ["net record-route ecmp", "cbgp_valid_net_record_route_ecmp"];

# -----[ cbgp_valid_net_record_route_ecmp ]--------------------------
# Test that record-route enumerates all possible paths
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#   - R4 (1.0.0.4, AS1)
#   - IGP weight equals 1 on all links
#
# Topology:
#
#     *-- R2 --*
#    /          \
#   R1          R4
#    \          /
#     *-- R3 --*
#
# Scenario:
#   * Record route from node 1.0.0.1 towards 1.0.0.4 w/ ECMP
#   * Check that two paths are found
#   * Check that path 0 goes through 1.0.0.2
#   * Check that path 1 goes through 1.0.0.3
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_ecmp($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4");
  $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4");
  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");
  my $traces= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.4", -ecmp=>1);
  return TEST_FAILURE
    if (scalar(@$traces) != 2);

  # Note: the path through 1.0.0.2 is returned first. This is
  #       deterministic and due to the internal ordering of routing
  #       table entries in c-bgp.
	
  return TEST_FAILURE
    if (!check_recordroute($traces->[0],
			   -status=>'SUCCESS',
			   -path=>[[0,'1.0.0.1'],
				   [1,'1.0.0.2'],
				   [2,'1.0.0.4']]));
  return TEST_FAILURE
    if (!check_recordroute($traces->[1],
			   -status=>'SUCCESS',
			   -path=>[[0,'1.0.0.1'],
				   [1,'1.0.0.3'],
				   [2,'1.0.0.4']]));
  return TEST_SUCCESS;
}

