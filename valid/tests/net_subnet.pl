return ["net subnet", "cbgp_valid_net_subnet", $topo];

# -----[ cbgp_valid_net_subnet ]-------------------------------------
# Create a topology composed of 2 nodes and 2 subnets (1 transit and
# one stub). Check that the links are correcly setup. Check the link
# attributes.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#
# Topology:
#
#   R1 ----(192.168.0/24)---- R2 ----(192.168.1/24)
#      .1                  .2    .2
#
# Scenario:
#   * Check link attributes
# -------------------------------------------------------------------
sub cbgp_valid_net_subnet($$) {
  my ($cbgp, $topo)= @_;
  my $msg;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
  $cbgp->send_cmd("net add subnet 192.168.1/24 stub");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24");
  $cbgp->send_cmd("net link 1.0.0.1 192.168.0.1/24 igp-weight 123");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.2/24");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.0.2/24 igp-weight 321");
  $cbgp->send_cmd("net add link 1.0.0.2 192.168.1.2/24");
  $cbgp->send_cmd("net link 1.0.0.2 192.168.1.2/24 igp-weight 456");

  return TEST_FAILURE
    if (!cbgp_check_link($cbgp, '1.0.0.1', '192.168.0.0/24',
			 -type=>C_IFACE_PTMP,
			 -weight=>123));
  return TEST_FAILURE
    if (!cbgp_check_link($cbgp, '1.0.0.2', '192.168.0.0/24',
			 -type=>C_IFACE_PTMP,
			 -weight=>321));
  return TEST_FAILURE
    if (!cbgp_check_link($cbgp, '1.0.0.2', '192.168.1.0/24',
			 -type=>C_IFACE_PTMP,
			 -weight=>456));

  # Create a /32 subnet: should fail
  $msg= cbgp_check_error($cbgp, "net add subnet 1.2.3.4/32 transit");
  return TEST_FAILURE
    if (!check_has_error($msg, $CBGP_ERROR_INVALID_SUBNET));
  return TEST_SUCCESS;
}

