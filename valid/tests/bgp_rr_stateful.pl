return ["bgp RR stateful",
	"cbgp_valid_bgp_rr_stateful"];

# -----[ cbgp_valid_bgp_rr_stateful ]--------------------------------
# Test ability to not propagate a route if its attributes have not
# changed. In the case of route-reflectors, this could occur if two
# routes with the same originator-id and cluster-id-list are received,
# and the second one changes the best route. In a stateful BGP
# implementation, no  update should be sent to neighbors after the
# second route is received since the attributes have not changed.
#
# Setup:
#   - R1 (1.0.0.1, AS1), virtual, rr-client
#   - R2 (1.0.0.2, AS1), RR, cluster-id 1
#   - R3 (1.0.0.3, AS1), RR, cluster-id 1
#   - R4 (1.0.0.4, AS1), RR, cluster 1.0.0.4
#   - R5 (1.0.0.5, AS1), rr-client, virtual
#
# Topology:
#
#      *-- R2 --*
#     /          \
#   (R1)          R4 --O-- (R5)
#     \          /     |
#      *-- R3 --*     tap
#
# Scenario:
#   * R1 sends to R3 a route towards 255/8
#   * R3 propagates the route to R4 which selects this route as best
#   * R1 sends to R2 a route towards 255/8 (with same attributes as
#       in [1])
#   * R4 propagates an update to R5
#   * R2 propagates the route to R4 which selects this route as best
#       since the address of R2 is lower than that of R3 (final tie-break)
#   * Check that R4 does not propagate an update to R5
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_stateful($) {
  my ($cbgp)= @_;
  my $mrt_record_file= get_tmp_resource("cbgp-mrt-record-1.0.0.5");

  unlink $mrt_record_file;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net add node 1.0.0.5");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net node 1.0.0.5 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.5");
  $cbgp->send_cmd("net link 1.0.0.4 1.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");

  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tset cluster-id 1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
  $cbgp->send_cmd("\tpeer 1.0.0.4 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("\tset cluster-id 1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
  $cbgp->send_cmd("\tpeer 1.0.0.4 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.4");
  $cbgp->send_cmd("bgp router 1.0.0.4");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
  $cbgp->send_cmd("\tpeer 1.0.0.5 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.5 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.5 record $mrt_record_file");
  $cbgp->send_cmd("\tpeer 1.0.0.5 up");
  $cbgp->send_cmd("\texit");

  cbgp_recv_update($cbgp, "1.0.0.3", 1, "1.0.0.1",
		   "255/8||IGP|1.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  cbgp_recv_update($cbgp, "1.0.0.2", 1, "1.0.0.1",
		   "255/8||IGP|1.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  cbgp_checkpoint($cbgp);

  open(MRT_RECORD, "<$mrt_record_file") or
    die "unable to open \"$mrt_record_file\": $!";
  my $line_count= 0;
  while (<MRT_RECORD>) {
    $line_count++;
  }
  close(MRT_RECORD);

  #  unlink $mrt_record_file;

  if ($line_count != 1) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

