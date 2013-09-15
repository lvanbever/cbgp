return ["net static routes errors", "cbgp_valid_net_static_routes_errors"];

# -----[ cbgp_valid_net_static_routes_errors ]-----------------------
# Check that adding a static route with a next-hop that cannot be
# joined through the interface generates an error.
#
# Setup:
#
# Scenario:
#   * try to add a static route towards R2 in R1 with R1's loopback
#     as outgoing interface.
#   * check that an error is raised
#   * try to add a static route towards R2
#   * check that an error is raised
# -------------------------------------------------------------------
sub cbgp_valid_net_static_routes_errors($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
  # Should not be able to add a route with R1's loopbask as outgoing
  # interface.
  my $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 ".
			    "route add --oif=1.0.0.1 1.0.0.2/32 10");
  return TEST_FAILURE
    if (!check_has_error($msg, "incompatible interface"));
  # Should not be able to add a route through an unexisting interface
  $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 ".
			 "route add --oif=1.0.0.2 1.0.0.2/32 10");
  return TEST_FAILURE
    if (!check_has_error($msg, "unknown interface"));
  # Should not be able to add a route with a next-hop that is not
  # reachable through the outgoing interface.
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 ".
			 "route add --gw=1.0.0.3 --oif=1.0.0.2 1.0.0.2/32 10");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_ROUTE_NH_UNREACH));
  # Should not be able to add a route through a subnet, with a next-hop
  # equal to the outgoing interface
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24");
  $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 route add ".
			 "--gw=192.168.0.1 --oif=192.168.0.1/24 1.0.0.2/32 10");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_ROUTE_NH_UNREACH));
  # Should not be able to add route through a subnet that cannot reach
  # the next-hop
  $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 route add ".
			 "--gw=192.168.0.2 --oif=192.168.0.1/24 1.0.0.2/32 10");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_ROUTE_NH_UNREACH));
  return TEST_SUCCESS;
}

