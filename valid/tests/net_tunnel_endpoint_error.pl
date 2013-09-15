use strict;

return ["net tunnel endpoint error",
	"cbgp_valid_net_tunnel_endpoint_error"];

# -----[ cbgp_valid_net_tunnel_endpoint_error ]----------------------
# Check that an error is generated when a tunnel without a valid
# corresponding endpoint is probed.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#
# Topology:
#
#   R1 ----- R2
#
# Scenario:
#   * Trace route from R1 to R2 : should be {R1, R2}
#   * Create a tunnel from R1 to R2 (and NOT the other way around)
#   * Add a route in R1 to join R2 through the tunnel
#   * Trace route from R1 to R2: should be UNREACHABLE
# -------------------------------------------------------------------
sub cbgp_valid_net_tunnel_endpoint_error($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");

  my $trace= cbgp_record_route($cbgp, '1.0.0.1', '1.0.0.2');
  return TEST_FAILURE
    if (!(check_recordroute($trace,
			    -path=>[[0,'1.0.0.1'],
				    [1,'1.0.0.2']])));

  $cbgp->send_cmd("net node 1.0.0.1");
  $cbgp->send_cmd("\ttunnel add 255.0.0.2 255.0.0.1");
  $cbgp->send_cmd("\troute add 1.0.0.2/32 --oif=255.0.0.1 1");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("net domain 1 compute");

  $trace= cbgp_record_route($cbgp, '1.0.0.1', '1.0.0.2');
  return TEST_FAILURE
    if (!(check_recordroute($trace,
			    -status=>'UNREACH',
			    -path=>[[0,'1.0.0.1']])));

  return TEST_SUCCESS;
}
