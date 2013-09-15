use strict;

return ["bgp router record-route",
	"cbgp_valid_bgp_router_record_route"];

# -----[ cbgp_valid_bgp_router_record_route ]------------------------
# Create the given topology in C-BGP. Check that bgp record-route
# fails. Then, propagate the IGP routes. Check that record-route are
# successfull.
#
# TODO: check loop detection.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_router_record_route($) {
  my ($cbgp)= @_;

  my $topo= topo_3nodes_line();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("  add network 255/8");
  $cbgp->send_cmd("  add peer 2 1.0.0.2");
  $cbgp->send_cmd("  peer 1.0.0.2 up");
  $cbgp->send_cmd("  exit");
  $cbgp->send_cmd("bgp add router 2 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("  add peer 1 1.0.0.1");
  $cbgp->send_cmd("  peer 1.0.0.1 up");
  $cbgp->send_cmd("  add peer 3 1.0.0.3");
  $cbgp->send_cmd("  peer 1.0.0.3 up");
  $cbgp->send_cmd("  exit");
  $cbgp->send_cmd("bgp add router 3 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("  add peer 2 1.0.0.2");
  $cbgp->send_cmd("  peer 1.0.0.2 up");
  $cbgp->send_cmd("  exit");

  my $trace= cbgp_bgp_record_route($cbgp, "1.0.0.3", "255/8");
  if (!defined($trace) ||
      ($trace->[F_TR_STATUS] ne "UNREACHABLE")) {
    return TEST_FAILURE;
  }

  $cbgp->send_cmd("sim run");

  $trace= cbgp_bgp_record_route($cbgp, "1.0.0.3", "255/8");
  if (!defined($trace) ||
      ($trace->[F_TR_STATUS] ne "SUCCESS") ||
      !aspath_equals($trace->[F_TR_PATH], [3, 2, 1])) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

