return ["bgp options show-mode mrt", "cbgp_valid_bgp_options_showmode_mrt"];

# -----[ cbgp_valid_bgp_options_showmode_mrt ]-----------------------
# Test ability to show routes in MRT format.
#
# Setup:
#   - R1 (123.0.0.234, AS255)
#
# Scenario:
#   * R1 has local network prefix "192.168.0/24"
# -------------------------------------------------------------------
sub cbgp_valid_bgp_options_showmode_mrt($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 123.0.0.234");
  $cbgp->send_cmd("bgp add router 255 123.0.0.234");
  $cbgp->send_cmd("bgp router 123.0.0.234 add network 192.168.0/24");
  $cbgp->send_cmd("bgp options show-mode mrt");
  $cbgp->send_cmd("bgp router 123.0.0.234 show rib *");
  $cbgp->send_cmd("print \"done\\n\"");

  while ((my $result= $cbgp->expect(1)) ne "done") {
    (!bgp_route_parse_mrt($result)) and
      return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

