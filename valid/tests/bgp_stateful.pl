return ["bgp stateful", "cbgp_valid_bgp_stateful"];

# -----[ cbgp_valid_bgp_stateful ]-----------------------------------
# Test that BGP router avoids to propagate updates when not necessary.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#   - R4 (2.0.0.1, AS2)
#
# Topology:
#
#  R2
#    \
#     *-- R1 --- R4
#    /
#  R3
#
# Scenario:
#   * R3 advertises route R to R1
#   * R1 propagates to R4
#   * R2 advertises same route R to R1
#   * R1 prefers R2's route (lowest router-ID).
#     However, it does not propagate the route to R4 since the
#     route attributes are equal.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_stateful($) {
  my ($cbgp)= @_;
  my $mrt_record_file= get_tmp_resource("cbgp-record-2.0.0.1.mrt");

  unlink "$mrt_record_file";

  cbgp_topo_dp4($cbgp,
		["1.0.0.2", 1, 1, 1],
		["1.0.0.3", 1, 1, 1],
		["2.0.0.1", 2, 1, 1, "record $mrt_record_file"],
		["3.0.0.1", 3, 1, 1]);
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.2",
		   "255/16|3|IGP|3.0.0.1|0|0");
  cbgp_recv_update($cbgp, "1.0.0.1", 1, "1.0.0.3",
		   "255/16|3|IGP|3.0.0.1|0|0");
  $cbgp->send_cmd("sim run");
  cbgp_checkpoint($cbgp);

  open(MRT_RECORD, "<$mrt_record_file") or
    die "unable to open \"$mrt_record_file\"";
  my $line_count= 0;
  while (<MRT_RECORD>) {
    $line_count++;
  }
  close(MRT_RECORD);

  unlink "$mrt_record_file";

  if ($line_count != 1) {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}

