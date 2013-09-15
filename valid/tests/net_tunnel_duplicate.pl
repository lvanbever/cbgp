use strict;

return ["net tunnel duplicate", "cbgp_valid_net_tunnel_duplicate"];

# -----[ cbgp_valid_net_tunnel_duplicate ]---------------------------
# Check that it is not possible to add the same tunnel (i.e. with the
# same interface address) twice on the same node.
#
# Setup:
#   - R1 (1.0.0.1)
#
# Scenario:
#   * Add a tunnel to R1 with local endpoint address X
#   * Add a tunnel to R1 with local endpoint address X:
#     check that it fails
# -------------------------------------------------------------------
sub cbgp_valid_net_tunnel_duplicate($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");

  my $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 tunnel add 255.0.0.2 255.0.0.1");
  return TEST_FAILURE
    if (check_has_error($msg));

  $msg= cbgp_check_error($cbgp, "net node 1.0.0.1 tunnel add 255.0.0.3 255.0.0.1");

  return TEST_FAILURE
    if (!check_has_error($msg, "interface already exists"));

  return TEST_SUCCESS;
}
