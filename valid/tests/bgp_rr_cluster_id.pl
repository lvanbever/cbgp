return ["bgp RR set cluster-id",
	"cbgp_valid_bgp_rr_set_cluster_id"];

# -----[ cbgp_valid_bgp_rr_set_cluster_id ]--------------------------
# Check that the cluster-ID can be defined correctly (including
# large cluster-ids).
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_set_cluster_id($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 0.1.0.0");
  $cbgp->send_cmd("bgp add router 1 0.1.0.0");
  my $msg= cbgp_check_error($cbgp,
			    "bgp router 0.1.0.0 set cluster-id 1");
  return TEST_FAILURE
    if (defined($msg));

  # Maximum value (2^32)-1
  $msg= cbgp_check_error($cbgp,
			 "bgp router 0.1.0.0 set cluster-id 4294967295");
  return TEST_FAILURE
    if (defined($msg));

  # Larger than (2^32)-1
  $msg= cbgp_check_error($cbgp,
			 "bgp router 0.1.0.0 set cluster-id 4294967296");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_CLUSTERID_INVALID));

  return TEST_SUCCESS;
}

