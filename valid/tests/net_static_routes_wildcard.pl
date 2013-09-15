return ["net static routes wildcards", "cbgp_valid_net_static_routes_wildcards"];

# -----[ cbgp_valid_net_static_routes_wildcards ]--------------------
# Check ability to add and remove static routes using wildcards.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_net_static_routes_wildcards($)
  {
    my ($cbgp)= @_;
    my $trace;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");

    # Test with static towards each node: should succeed
    $cbgp->send_cmd("net node 1.0.0.1 route add --oif=1.0.0.2 1.0.0.2/32 10");
    $cbgp->send_cmd("net node 1.0.0.1 route add --oif=1.0.0.3 1.0.0.3/32 10");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] ne "SUCCESS") {
      $tests->debug("=> ERROR TEST(1)");
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.3");
    if ($trace->[F_TR_STATUS] ne "SUCCESS") {
      $tests->debug("=> ERROR TEST(2)");
      return TEST_FAILURE;
    }

    # Remove routes using wildcard: should succeed
    $cbgp->send_cmd("net node 1.0.0.1 route del 1.0.0.2/32 *");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
      $tests->debug("=> ERROR TEST(3)");
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.3");
    if ($trace->[F_TR_STATUS] ne "SUCCESS") {
      $tests->debug("=> ERROR TEST(4)");
      return TEST_FAILURE;
    }

    # Remove routes using wildcard: should succeed
    $cbgp->send_cmd("net node 1.0.0.1 route del 1.0.0.3/32 *");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
      $tests->debug("=> ERROR TEST(5)");
      return TEST_FAILURE;
    }
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.3");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
      $tests->debug("=> ERROR TEST(6)");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

