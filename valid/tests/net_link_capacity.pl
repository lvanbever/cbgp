return ["net link capacity", "cbgp_valid_net_link_capacity"];

# -----[ cbgp_valid_net_link_capacity ]------------------------------
# Check that the capacity of a link can be defined during it's
# creation. The capacity is assigned for both directions.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#
# Topology:
#   R1 ----- R2
#
# Scenario:
#   * Create R1, R2 and add link with option --bw=12345678
#   * Check that the capacity is assigned for both directions.
#   * Assign a load to R1->R2 and another to R2->R1
#   * Check that the load is correctly assigned
#   * Clear the load in one direction
#   * Check that the load is cleared in only one direction
# -------------------------------------------------------------------
sub cbgp_valid_net_link_capacity($)
  {
    my ($cbgp)= @_;
    my $info;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add link --bw=12345678 1.0.0.1 1.0.0.2");

    # Check load and capacity of R1->R2
    $info= cbgp_link_info($cbgp, '1.0.0.1', '1.0.0.2');
    if (!exists($info->{'capacity'}) || ($info->{'capacity'} != 12345678) ||
	!exists($info->{'load'}) || ($info->{'load'} != 0)) {
      $tests->debug("Error: no/invalid capacity/load for link R1->R2");
      return TEST_FAILURE;
    }

    # Check load and capacity of R2->R1
    $info= cbgp_link_info($cbgp, '1.0.0.2', '1.0.0.1');
    if (!exists($info->{'capacity'}) || ($info->{'capacity'} != 12345678) ||
	!exists($info->{'load'}) || ($info->{'load'} != 0)) {
      $tests->debug("Error: no/invalid capacity for link R2->R1");
      return TEST_FAILURE;
    }

    # Add load to R1->R2
    $cbgp->send_cmd("net node 1.0.0.1 iface 1.0.0.2 load add 589");
    $info= cbgp_link_info($cbgp, '1.0.0.1', '1.0.0.2');
    if (!exists($info->{'load'}) ||
	($info->{'load'} != 589)) {
      $tests->debug("Error: no/invalid load for link R1->R2");
      return TEST_FAILURE;
    }

    # Load of R1->R2 should be cleared
    $cbgp->send_cmd("net node 1.0.0.1 iface 1.0.0.2 load clear");
    $info= cbgp_link_info($cbgp, '1.0.0.1', '1.0.0.2');
    if (!exists($info->{'load'}) ||
	($info->{'load'} != 0)) {
      $tests->debug("Error: load of link R1->R2 should be cleared");
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

