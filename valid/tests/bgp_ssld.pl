return ["bgp ssld", "cbgp_valid_bgp_ssld"];

# -----[ cbgp_valid_bgp_ssld ]---------------------------------------
# Test sender-side loop detection.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (2.0.0.1, AS2)
#   - R3 (3.0.0.1, AS3)
#
# Topology:
#
#   R3 ----- R1 ----- R2
#
# Scenario:
#   * R3 announces to R1 a route towards 255/8 with an
#       AS-Path [AS3 AS2 AS1].
#   * Check that R1 do not advertise the route to R2.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_ssld($) {
  my ($cbgp)= @_;

  die if $cbgp->send("net add domain 1 igp\n");
  die if $cbgp->send("net add node 1.0.0.1\n");
  die if $cbgp->send("net add node 2.0.0.1\n");
  die if $cbgp->send("net add node 3.0.0.1\n");
  die if $cbgp->send("net add link 1.0.0.1 2.0.0.1\n");
  die if $cbgp->send("net link 1.0.0.1 2.0.0.1 igp-weight 10\n");
  die if $cbgp->send("net link 2.0.0.1 1.0.0.1 igp-weight 10\n");
  die if $cbgp->send("net add link 1.0.0.1 3.0.0.1\n");
  die if $cbgp->send("net link 1.0.0.1 3.0.0.1 igp-weight 10\n");
  die if $cbgp->send("net link 3.0.0.1 1.0.0.1 igp-weight 10\n");
  die if $cbgp->send("net node 1.0.0.1 domain 1\n");
  die if $cbgp->send("net node 2.0.0.1 domain 1\n");
  die if $cbgp->send("net node 3.0.0.1 domain 1\n");
  die if $cbgp->send("net domain 1 compute\n");
  die if $cbgp->send("bgp add router 1 1.0.0.1\n");
  die if $cbgp->send("bgp router 1.0.0.1\n");
  die if $cbgp->send("\tadd peer 3 3.0.0.1\n");
  die if $cbgp->send("\tpeer 3.0.0.1 virtual\n");
  die if $cbgp->send("\tpeer 3.0.0.1 up\n");
  die if $cbgp->send("\tadd peer 2 2.0.0.1\n");
  die if $cbgp->send("\tpeer 2.0.0.1 up\n");
  die if $cbgp->send("\texit\n");
  die if $cbgp->send("bgp add router 2 2.0.0.1\n");
  die if $cbgp->send("bgp router 2.0.0.1\n");
  die if $cbgp->send("\tadd peer 1 1.0.0.1\n");
  die if $cbgp->send("\tpeer 1.0.0.1 up\n");
  die if $cbgp->send("\texit\n");
  $cbgp->send_cmd("sim run");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "255/8|3 2 255|IGP|3.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "3.0.0.1",
		   "254/8|3 4 255|IGP|3.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  my $rib= cbgp_get_rib($cbgp, "2.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "254/8"));
  return TEST_FAILURE
    if (check_has_bgp_route($rib, "255/8"));

  return TEST_SUCCESS;
}

