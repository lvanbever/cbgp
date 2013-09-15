return ["net static routes duplicate", "cbgp_valid_net_static_routes_duplicate"];

# -----[ cbgp_valid_net_static_routes_duplicate ]--------------------
# Check that adding multiple routes towards the same prefix generates
# an error.
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_net_static_routes_duplicate($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  # Adding a single route: should succeed
  my $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 ".
			    "route add --oif=1.0.0.2 1.0.0.2/32 10");
  return TEST_FAILURE
    if (check_has_error($msg));
  # Adding a route through same interface: should fail
  $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 ".
			 "route add --oif=1.0.0.2 1.0.0.2/32 10");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_ROUTE_EXISTS));
  # Adding a route through other interface: should succeed
  $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 ".
			 "route add --oif=1.0.0.3 1.0.0.2/32 10");
  return TEST_FAILURE
    if (check_has_error($msg));
  return TEST_SUCCESS;
}

