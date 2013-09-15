return ["bgp route-map duplicate",
		 "cbgp_valid_bgp_route_map_duplicate"];

# -----[ cbgp_valid_bgp_route_map_duplicate ]------------------------
# Check that an error message is issued if an attempt is made to
# define two route-maps with the same name.
#
# Setup:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_route_map_duplicate($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("bgp route-map RM_LP_100");
  $cbgp->send_cmd("  exit");
  my $msg= cbgp_check_error($cbgp, "bgp route-map RM_LP_100");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_ROUTEMAP_EXISTS));

  return TEST_SUCCESS;
}

