return ["net link-ptp", "cbgp_valid_net_link_ptp"];

# -----[ cbgp_valid_net_link_ptp ]-----------------------------------
# -------------------------------------------------------------------
sub cbgp_valid_net_link_ptp($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add link-ptp 1.0.0.1 192.168.0.1/30 ".
		  "1.0.0.2 192.168.0.2/30");
  $cbgp->send_cmd("net add link-ptp 1.0.0.1 192.168.0.5/30 ".
		  "1.0.0.3 192.168.0.6/30");
  $cbgp->send_cmd("net add link-ptp 1.0.0.2 192.168.0.9/30 ".
		  "1.0.0.3 192.168.0.10/30");

  return TEST_SUCCESS;
}

