return ["bgp RR decision process (cluster-id-list)",
	"cbgp_valid_bgp_rr_dp_cluster_id_list"];

# -----[ cbgp_valid_bgp_rr_dp_cluster_id_list ]----------------------
# Test ability to break ties based on the cluster-id-list length.
#
# Setup:
#   - R1 (1.0.0.1, AS1), virtual peer of R2 and R3
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#   - R4 (1.0.0.4, AS1) rr-client of R2
#   - R5 (1.0.0.5, AS1) rr-client of R3 and R4
#
# Topology:
#
#      *-- R2 ---- R3 --*
#     /                  \
#   R1                    R5
#     \                  /
#      *-- R4 ----------*
#
# Scenario:
#   * Advertise 255/8 from R1 to R2 with community 255:1
#   * Advertise 255/8 from R1 to R4 with community 255:2
#   * Check that R5 has received two routes towards 255/8
#   * Check that R5's best route has community 255:2 (selected based
#     on the smallest cluster-id-list)
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_dp_cluster_id_list($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.5");
  $cbgp->send_cmd("net node 1.0.0.5 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.5");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.5");
  $cbgp->send_cmd("net link 1.0.0.4 1.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");

  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
  $cbgp->send_cmd("\tpeer 1.0.0.5 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.5 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.4");
  $cbgp->send_cmd("bgp router 1.0.0.4");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
  $cbgp->send_cmd("\tpeer 1.0.0.5 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.5 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.5");
  $cbgp->send_cmd("bgp router 1.0.0.5");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
  $cbgp->send_cmd("\tpeer 1.0.0.4 up");
  $cbgp->send_cmd("\texit");

  cbgp_recv_update($cbgp, "1.0.0.2", 1, "1.0.0.1",
		   "255/8|2|IGP|1.0.0.1|0|0|255:1");
  cbgp_recv_update($cbgp, "1.0.0.4", 1, "1.0.0.1",
		   "255/8|2|IGP|1.0.0.1|0|0|255:2");

  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib_custom($cbgp, "1.0.0.5");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -community=>["255:2"]));
  return TEST_SUCCESS;
}

