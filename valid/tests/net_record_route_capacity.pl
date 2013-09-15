return ["net record-route capacity", "cbgp_valid_net_record_route_capacity"];

# -----[ cbgp_valid_net_record_route_capacity ]----------------------
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
#       2k        2k
#   R1 ----- R2 ----- R3
#            |\  100
#            | \+---- R4
#            |    1k
#            +------- R5
#
# Scenario:
#   * Setup topology, link capacities and compute routes. Perform
#     record-route from R1 to all nodes.
#   * Check that capacity(R1->R2) = 2000
#   * Check that capacity(R1->R3) = 2000
#   * Check that capacity(R1->R4) = 100
#   * Check that capacity(R1->R5) = 1000
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_capacity($)
  {
    my ($cbgp)= @_;
    my $trace;

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
    $cbgp->send_cmd("net add link --bw=2000 1.0.0.1 1.0.0.2");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link --bw=3000 1.0.0.2 1.0.0.3");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link --bw=100 1.0.0.2 1.0.0.4");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link --bw=1000 1.0.0.2 1.0.0.5");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.5 igp-weight --bidir 1");
    $cbgp->send_cmd("net domain 1 compute");

    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2", -capacity=>1);
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"SUCCESS",
			     -capacity=>2000));
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.3", -capacity=>1);
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"SUCCESS",
			     -capacity=>2000));
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.4", -capacity=>1);
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"SUCCESS",
			     -capacity=>100));
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.5", -capacity=>1);
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"SUCCESS",
			     -capacity=>1000));
    return TEST_SUCCESS;
  }


