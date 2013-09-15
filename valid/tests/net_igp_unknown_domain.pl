return ["net igp unknown domain", "cbgp_valid_net_igp_unknown_domain"];

# -----[ cbgp_valid_net_igp_unknown_domain ]-------------------------
# Check that adding a node to an unexisting domain fails.
#
# Scenario:
#   * Create a node 0.1.0.0
#   * Try to add the node to a domain 1 (not created)
#   * Check the error message
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_unknown_domain($$) {
  my ($cbgp, $topo)= @_;

  $cbgp->send_cmd("net add node 0.1.0.0");
  my $msg= cbgp_check_error($cbgp, "net node 0.1.0.0 domain 1");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_DOMAIN_UNKNOWN));
  return TEST_SUCCESS;
}

