return ["bgp RR originator ssld",
	"cbgp_valid_bgp_rr_ossld"];

# -----[ cbgp_valid_bgp_rr_ossld ]------------------------------------
# Test route-reflector ability to avoid redistribution to originator.
#
# Setup:
#   - R1 (1.0.0.1, AS1), rr-client, virtual
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#
#   Note: R2 and R3 are in different clusters (default)
#
# Topology:
#
#   R2 ------ R3
#     \      /
#      \    /
#       (R1)
#
# Scenario:
#   * R1 announces 255/8 to R2
#   * R2 propagates to R3
#   * Check that R3 has received the route and that it does not
#     it send back to R1
# --------------------------------------------------------------------
sub cbgp_valid_bgp_rr_ossld($) {
  my ($cbgp)= @_;
  my $mrt_record_file= get_tmp_resource("cbgp-mrt-record-1.0.0.1");

  unlink $mrt_record_file;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");

  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 rr-client");
  $cbgp->send_cmd("\tpeer 1.0.0.1 virtual");
  $cbgp->send_cmd("\tpeer 1.0.0.1 record $mrt_record_file");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");

  cbgp_recv_update($cbgp, "1.0.0.2", 1, "1.0.0.1",
		   "255/8||IGP|1.0.0.1|0|0");
  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib_mrt($cbgp, "1.0.0.3");
  if (!exists($rib->{"255.0.0.0/8"})) {
    return TEST_FAILURE;
  }

  open(MRT_RECORD, "<$mrt_record_file") or
    die "unable to open \"$mrt_record_file\"";
  my $line_count= 0;
  while (<MRT_RECORD>) {
    $line_count++;
  }
  close(MRT_RECORD);

  unlink $mrt_record_file;

  if ($line_count != 0) {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

