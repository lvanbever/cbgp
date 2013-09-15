return ["net tunnel (traceroute-local)", "cbgp_valid_net_tunnel_traceroute"];

# -----[ cbgp_valid_net_tunnel_traceroute ]--------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_net_tunnel_traceroute($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1");
  $cbgp->send_cmd("  tunnel add 1.0.0.2 255.0.0.1");
  $cbgp->send_cmd("  exit");
  my $trace= cbgp_traceroute($cbgp, '1.0.0.1', '255.0.0.1');
  return TEST_FAILURE
    if (!check_traceroute($trace, -status=>C_TRACEROUTE_STATUS_REPLY));
  return TEST_SUCCESS;
}

