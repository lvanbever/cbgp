return ["bgp RR", "cbgp_valid_bgp_rr"];

# -----[ cbgp_valid_bgp_rr ]-----------------------------------------
# Test basic route-reflection mechanisms.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1) route-reflector
#   - R3 (1.0.0.3, AS1) client of R3
#   - R4 (1.0.0.4, AS1) peer of R4 (non-client)
#   - R5 (1.0.0.5, AS1) peer of R2 (non-client)
#   - R6 (2.0.0.1, AS2) virtual peer of R1
#   - R7 (3.0.0.1, AS3)
#   - R8 (4.0.0.1, AS4)
#
# Topology:
#
#                       *---- R7
#                      /
#   (R6) ---- R1 ---- R2 ---- R3 ---- R4
#                      \       \
#                       \        *---- R8
#                        \
#                         *---- R5
#
# Scenario:
#   * R6 advertise route towards 255/8
#   * Check that R1, R2 and R3 have the route
#   * Check that R4 ad R5 do not have the route
#   * Check that R2 has set the originator-ID to the router-ID of R1
#   * Check that the route received by R3 has a cluster-ID-list
#     containing the router-ID of R2 (i.e. 1.0.0.2)
#   * Check that R7 and R8 have received the route and no
#     RR-attributes (Originator-ID and Cluster-ID-List) are present
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr($) {
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
  $cbgp->send_cmd("net add node 2.0.0.1");
  $cbgp->send_cmd("net node 2.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 3.0.0.1");
  $cbgp->send_cmd("net node 3.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 4.0.0.1");
  $cbgp->send_cmd("net node 4.0.0.1 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1");
  $cbgp->send_cmd("net link 1.0.0.1 2.0.0.1 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.5");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 3.0.0.1");
  $cbgp->send_cmd("net link 1.0.0.2 3.0.0.1 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.3 4.0.0.1");
  $cbgp->send_cmd("net link 1.0.0.3 4.0.0.1 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");

  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\tadd peer 2 2.0.0.1");
  $cbgp->send_cmd("\tpeer 2.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 2.0.0.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
  $cbgp->send_cmd("\tpeer 1.0.0.5 up");
  $cbgp->send_cmd("\tadd peer 3 3.0.0.1");
  $cbgp->send_cmd("\tpeer 3.0.0.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
  $cbgp->send_cmd("\tpeer 1.0.0.4 up");
  $cbgp->send_cmd("\tadd peer 4 4.0.0.1");
  $cbgp->send_cmd("\tpeer 4.0.0.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.4");
  $cbgp->send_cmd("bgp router 1.0.0.4");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.5");
  $cbgp->send_cmd("bgp router 1.0.0.5");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 3 3.0.0.1");
  $cbgp->send_cmd("bgp router 3.0.0.1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 4 4.0.0.1");
  $cbgp->send_cmd("bgp router 4.0.0.1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\texit");

  cbgp_recv_update($cbgp, "1.0.0.1", 1, "2.0.0.1",
		   "255/8|2|IGP|2.0.0.1|0|0");

  $cbgp->send_cmd("sim run");

  my $rib;
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8"));
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.2");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8"));
  $rib= cbgp_get_rib_custom($cbgp, "1.0.0.3");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -originator=>"1.0.0.1",
			     -clusterlist=>["1.0.0.2"]));
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.4");
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255/8"));
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.5");
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255/8"));
  $rib= cbgp_get_rib_custom($cbgp, "3.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -clusterlist=>undef,
			     -originator=>undef));
  $rib= cbgp_get_rib_custom($cbgp, "4.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -clusterlist=>undef,
			     -originator=>undef));
  return TEST_SUCCESS;
}


