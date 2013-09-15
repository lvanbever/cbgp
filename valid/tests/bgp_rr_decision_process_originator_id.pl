return ["bgp RR decision process (originator-id)",
	"cbgp_valid_bgp_rr_dp_originator_id"];

# -----[ cbgp_valid_bgp_rr_dp_originator_id ]------------------------
# Check that a BGP router that receives two equal routes will break
# the ties based on their Originator-ID.
#
# Setup:
#   - R1 (1.0.0.1, AS1), virtual
#   - R2 (1.0.0.2, AS1), virtual
#   - R3 (1.0.0.3, AS1, RR)
#   - R4 (1.0.0.4, AS1, RR)
#   - R5 (1.0.0.5, AS1, RR)
#
# Topology:
#
#    R1 ----- R4
#               \
#                 R5
#               /
#    R2 ----- R3
#
# Scenario:
#   * Inject 255/8 to R4, from R1
#   * Inject 255/8 to R3, from R2
#   * Check that XXX
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_dp_originator_id($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net add node 1.0.0.5");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net node 1.0.0.5 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.5");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.5");
  $cbgp->send_cmd("net link 1.0.0.4 1.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp add router 1 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.2 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
  $cbgp->send_cmd("\tpeer 1.0.0.5 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 1 1.0.0.4");
  $cbgp->send_cmd("bgp router 1.0.0.4");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.5");
  $cbgp->send_cmd("\tpeer 1.0.0.5 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp add router 1 1.0.0.5");
  $cbgp->send_cmd("bgp router 1.0.0.5");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.4");
  $cbgp->send_cmd("\tpeer 1.0.0.4 up");
  $cbgp->send_cmd("\texit");

  cbgp_recv_update($cbgp, "1.0.0.4", 1, "1.0.0.1",
		   "255/8||IGP|1.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.3", 1, "1.0.0.2",
		   "255/8||IGP|1.0.0.2|0|0");
  $cbgp->send_cmd("sim run");
  my $rib;
  my @routes;
  $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.5", "255/8");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -originator=>"1.0.0.1"));

  return TEST_SUCCESS;
}

