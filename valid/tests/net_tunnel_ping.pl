return ["net tunnel (ping-local)", "cbgp_valid_net_tunnel_ping"];

# -----[ cbgp_valid_net_tunnel_ping ]--------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_tunnel_ping($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1");
  $cbgp->send_cmd("  tunnel add 1.0.0.2 255.0.0.1");
  $cbgp->send_cmd("  exit");
  my $ping= cbgp_ping($cbgp, '1.0.0.1', '255.0.0.1');
  return TEST_FAILURE
    if (!check_ping($ping, -status=>C_PING_STATUS_REPLY));
  return TEST_SUCCESS;
}

