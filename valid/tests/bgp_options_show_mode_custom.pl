return ["bgp options show-mode custom", "cbgp_valid_bgp_options_showmode_custom"];

# -----[ cbgp_valid_bgp_options_showmode_custom ]--------------------
# Test ability to show routes in custom format.
#
# Setup:
#   - R1 (123.0.0.234, AS255)
#
# Scenario:
#   * R1 has local network prefix "192.168.0/24"
# -------------------------------------------------------------------
sub cbgp_valid_bgp_options_showmode_custom($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 123.0.0.234");
  $cbgp->send_cmd("bgp add router 255 123.0.0.234");
  $cbgp->send_cmd("bgp router 123.0.0.234 add network 192.168.0/24");
  $cbgp->send_cmd("bgp options show-mode \"ROUTE:%i;%a;%p;%P;%C;%O;%n\"");
  $cbgp->send_cmd("bgp router 123.0.0.234 show rib *");
  $cbgp->send_cmd("print \"done\\n\"");

  while ((my $result= $cbgp->expect(1)) ne "done") {
    chomp $result;
    my @fields= split /\:/, $result, 2;
    if ($fields[0] ne "ROUTE") {
      return TEST_FAILURE;
    }

    @fields= split /;/, $fields[1];

    my $peer_ip= shift @fields;
    my $peer_as= shift @fields;
    my $prefix= shift @fields;
    my $path= shift @fields;
    my $cluster_id_list= shift @fields;
    my $originator_id= shift @fields;
    my $nexthop= shift @fields;

    if ($peer_ip ne "LOCAL") {
      return TEST_FAILURE;
    }
    if ($peer_as ne "LOCAL") {
      return TEST_FAILURE;
    }
    if ($prefix ne "192.168.0.0/24") {
      return TEST_FAILURE;
    }
    if ($path ne "") {
      return TEST_FAILURE;
    }
    if ($nexthop ne "123.0.0.234") {
      return TEST_FAILURE;
    }
  }
  return TEST_SUCCESS;
}

