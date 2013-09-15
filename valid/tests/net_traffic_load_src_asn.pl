use strict;

return ["net traffic load (src=asn)",
	"cbgp_valid_net_traffic_load_src_asn"];

# -----[ cbgp_valid_net_traffic_load_src_asn ]-----------------------
# Check the ability to load a traffic matrix from a file and to load
# the links traversed by traffic. The traffic matrix used in this
# test specifies the source node based on an AS number. The
# destination is an IP address.
#
#   - AS1 provider of {AS3, AS4}
#   - AS2 provider of {AS3, AS4, AS7}
#   - AS3 provider of {AS5, AS6}
#   - AS4 provider of {AS6, AS7}
#   - AS5
#   - AS6
#   - AS7
#
# Topology:
#
#     1 --- 2
#     | \ / |\
#     |  \  | \
#     | / \ |  |
#     3 --- 4  |
#    /\    /\  |
#   /  \  /  \ |
#  /    \/    \|
#  5     6     7
#
# Scenario:
#   * Announce prefix 255/8 from AS6
#   * Load flows from file
#   * from AS1: 100
#   * from AS3: 200
#   * from AS4: 400
#   * from AS5: 800
#   * Check that the load on AS3->AS6 equals 1100
#   * Check that the load on AS4->AS6 equals 400
#
# Resources:
#   [as-level-topo-small.txt]
#   [valid-net-tm-src-asn.tm]
# -------------------------------------------------------------------
sub cbgp_valid_net_traffic_load_src_asn($) {
  my ($cbgp)= @_;
  my $msg;
  my $info;

  my $filename= get_resource("valid-net-tm-src-asn.tm");
  (-e $filename) or return TEST_SKIPPED;
  my $topo= get_resource("as-level-topo-small.txt");
  (-e $topo) or return TEST_SKIPPED;

  $cbgp->send_cmd("bgp topology load \"$topo\"");
  $cbgp->send_cmd("bgp topology install");
  $cbgp->send_cmd("bgp topology policies");
  $cbgp->send_cmd("bgp topology run");

  $cbgp->send_cmd("bgp router AS6 add network 255/8");
  $cbgp->send_cmd("sim run");

  $msg= cbgp_check_error($cbgp, "net traffic load --src=asn \"$filename\"");
  return TEST_FAILURE
    if (check_has_error($msg));

  $info= cbgp_link_info($cbgp, "AS4", "AS6");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>400));

  $info= cbgp_link_info($cbgp, "AS3", "AS6");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>1100));

  return TEST_SUCCESS;
}
