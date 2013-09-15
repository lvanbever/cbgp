use strict;

return ["net record-route tunnel",
	"cbgp_valid_net_record_route_tunnel"];

# -----[ cbgp_valid_net_record_route_tunnel ]------------------------
# Check that it is possible to trace the path into a tunnel.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#
# Topology:
#
#   R1 ----- R3 ----- R2
#             |
#             |
#            R4
#
# Scenario:
#   * Trace route from R1 to R2 : should be {R1, R3, R2}
#   * Create a tunnel from R1 to R4 (and the other way around)
#   * Add a route in R1 to join R2 through the tunnel
#   * Trace route from R1 to R2 with tunnel details:
#     should be {R1, [R1, R3, R4] R4, R3, R2}
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_tunnel($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4");

  # IGP Domain 1
  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");

  # Default route from R1 to R2 should be R1 R3 R2
  my $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2");
  return TEST_FAILURE
    if (!check_recordroute($trace,
			   -path=>[[1,'1.0.0.3'],
				   [2,'1.0.0.2']]));

  # Create tunnel from R1 to R4 and from R4 to R1
  $cbgp->send_cmd("net node 1.0.0.1");
  $cbgp->send_cmd("\ttunnel add 255.0.0.2 255.0.0.1");
  $cbgp->send_cmd("\troute add 1.0.0.2/32 --oif=255.0.0.1 1");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("net node 1.0.0.4");
  $cbgp->send_cmd("\ttunnel add 255.0.0.1 255.0.0.2");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("net domain 1 compute");

  # Default route from R1 to R2 should be R1 R3 R2
  $trace= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.2", -tunnel=>1);
  return TEST_FAILURE
    if (!check_recordroute($trace,
			   -path=>[[0,'1.0.0.1'],
				   [1,[[0,'1.0.0.1'],
				       [1,'1.0.0.3'],
				       [2,'1.0.0.4']]],
				   [2,'1.0.0.4'],
				   [3,'1.0.0.3'],
				   [4,'1.0.0.2']]));

  return TEST_SUCCESS;
}
