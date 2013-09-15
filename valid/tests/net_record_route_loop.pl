return ["net record-route loop", "cbgp_valid_net_record_route_loop"];

# -----[ cbgp_valid_net_record_route_loop ]--------------------------
# Test on the quick loop detection. A normal record-route does not
# detect loop. The basic behavior is to wait until TTL expiration.
#
# Setup:
#   - R1 (1.0.0.1, AS1), NH for 2.0.0.1 = 1.0.0.2
#   - R2 (1.0.0.2, AS1), NH for 2.0.0.1 = 1.0.0.1
#
# Topology:
#
#   R1 -- R2
#
# Scenario:
#   * Record route from node 1.0.0.1 towards 2.0.0.1
#   * Success if the result contains LOOP (instead of TOO_LONG)
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_loop($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");

    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");

    $cbgp->send_cmd("net node 1.0.0.1 route add --oif=1.0.0.2 2.0.0.1/32 1");
    $cbgp->send_cmd("net node 1.0.0.2 route add --oif=1.0.0.1 2.0.0.1/32 1");

    $cbgp->send_cmd("net domain 1 compute");

    my $trace;

    $trace= cbgp_record_route($cbgp, "1.0.0.1", "2.0.0.1");
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"UNREACH"));

    $trace= cbgp_record_route($cbgp, "1.0.0.1", "2.0.0.1",
					 -checkloop=>1);
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"LOOP"));
    return TEST_SUCCESS;
  }

