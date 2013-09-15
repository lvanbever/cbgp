return ["net subnet misc", "cbgp_valid_net_subnet_misc", $topo];

# -----[ cbgp_valid_net_subnet_misc ]--------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_subnet_misc($) {
  my ($cbgp)= @_;
  my $msg;
  my $ifaces;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add node 2.0.0.1");
  $cbgp->send_cmd("net add node 192.168.0.1");
  $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
  $cbgp->send_cmd("net add subnet 2.0.0/24 transit");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.1/24");
  $cbgp->send_cmd("net add link 1.0.0.1 192.168.0.2/24");

  # Add a mtp link to 1.0.0/24: it should succeed
  $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 2.0.0.0/24");
  return TEST_FAILURE
    if (check_has_error($msg));

  $ifaces= cbgp_get_ifaces($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_iface($ifaces, "1.0.0.1/32",
			 -type=>C_IFACE_LOOPBACK));
  return TEST_FAILURE
    if (!check_has_iface($ifaces, "192.168.0.1/24",
			 -type=>C_IFACE_PTMP));
  return TEST_FAILURE
    if (!check_has_iface($ifaces, "192.168.0.2/24",
			 -type=>C_IFACE_PTMP));
  return TEST_FAILURE
    if (!check_has_iface($ifaces, "1.0.0.2/32",
			 -type=>C_IFACE_RTR));
  return TEST_FAILURE
    if (!check_has_iface($ifaces, "1.0.0.3/32",
			 -type=>C_IFACE_RTR));
  return TEST_FAILURE
    if (!check_has_iface($ifaces, "2.0.0.1/32",
			 -type=>C_IFACE_RTR));
  return TEST_FAILURE
    if (!check_has_iface($ifaces, "2.0.0.0/24",
			 -type=>C_IFACE_PTMP));

  # Add a ptp link to 192.168.0.1:
  #  - it should fail (default)
  #  - it should succeed if (IFACE_ID_ADDR_MASK is set in src/link-list.c)
  $msg= cbgp_check_error($cbgp, "net add link 1.0.0.1 192.168.0.1");
  return TEST_FAILURE
    if (!check_has_error($msg, "incompatible interface"));
  return TEST_SUCCESS;
}

