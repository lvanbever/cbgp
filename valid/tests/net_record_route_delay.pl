return ["net record-route delay", "cbgp_valid_net_record_route_delay"];

# -----[ cbgp_valid_net_record_route_delay ]-------------------------
# Check that record-route is able to record the maximum bandwidth
# available along a path.
#
# Setup:
#   - R1 (1.0.0.4)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#   - R5 (1.0.0.5)
#
# Topology:
#       10        1
#   R1 ----- R2 ----- R3
#            |\  100
#            | \+---- R4
#            |    1k
#            +------- R5
#
# Scenario:
#   * Setup topology, link delays and compute routes. Perform
#     record-route from R1 to all nodes.
#   * Check that delay(R1->R2) = 10
#   * Check that delay(R1->R3) = 11
#   * Check that delay(R1->R4) = 110
#   * Check that delay(R1->R5) = 1010
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_delay($) {
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
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2 --delay=10");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3 --delay=1");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4 --delay=100");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.5 --delay=1000");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");

  my $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2", -delay=>1);
  return TEST_FAILURE
    if (!check_recordroute($trace, -status=>"SUCCESS",
			   -delay=>10));
  $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.3", -delay=>1);
  return TEST_FAILURE
    if (!check_recordroute($trace, -status=>"SUCCESS",
			   -delay=>11));
  $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.4", -delay=>1);
  return TEST_FAILURE
    if (!check_recordroute($trace, -status=>"SUCCESS",
			   -delay=>110));
  $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.5", -delay=>1);
  return TEST_FAILURE
    if (!check_recordroute($trace, -status=>"SUCCESS",
			   -delay=>1010));
  return TEST_SUCCESS;
}

