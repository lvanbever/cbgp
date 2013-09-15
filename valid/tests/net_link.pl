return ["net link", "cbgp_valid_net_link", $topo];

# -----[ cbgp_valid_net_link ]---------------------------------------
# Create a topology composed of 3 nodes and 2 links. Check that the
# links are correctly created. Check the links attributes.
# -------------------------------------------------------------------
sub cbgp_valid_net_link($$) {
  my ($cbgp, $topo)= @_;
  $cbgp->send_cmd("net add node 1.0.0.1\n");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 123");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.1 igp-weight 123");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight 321");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.2 igp-weight 321");
  return TEST_FAILURE
    if (!cbgp_check_link($cbgp, '1.0.0.1', '1.0.0.2',
			 -type=>C_IFACE_RTR,
			 -weight=>123));
  return TEST_FAILURE
    if (!cbgp_check_link($cbgp, '1.0.0.2', '1.0.0.1',
			 -type=>C_IFACE_RTR,
			 -weight=>123));
  return TEST_FAILURE
    if (!cbgp_check_link($cbgp, '1.0.0.2', '1.0.0.3',
			 -type=>C_IFACE_RTR,
			 -weight=>321));
  return TEST_FAILURE
  if (!cbgp_check_link($cbgp, '1.0.0.3', '1.0.0.2',
		       -type=>C_IFACE_RTR,
		       -weight=>321));
  return TEST_SUCCESS;
  }

