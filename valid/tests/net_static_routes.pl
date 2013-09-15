return ["net static routes", "cbgp_valid_net_static_routes"];

# -----[ cbgp_valid_net_static_routes ]------------------------------
# Check ability to add and remove static routes.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_net_static_routes($)
  {
    my ($cbgp)= @_;
    my $trace;
    my $rt;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");

    # Test without static route: should fail
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
      return TEST_FAILURE;
    }

    # Test with static route: should succeed
    $cbgp->send_cmd("net node 1.0.0.1 route add --gw=1.0.0.2 --oif=1.0.0.2 1.0.0.2/32 10");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] ne "SUCCESS") {
      return TEST_FAILURE;
    }

    # Remove route and test: should fail
    $cbgp->send_cmd("net node 1.0.0.1 route del --gw=1.0.0.2 1.0.0.2/32 1.0.0.2");
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
    if ($trace->[F_TR_STATUS] eq "SUCCESS") {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

