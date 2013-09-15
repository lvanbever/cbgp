use strict;

return ["net traffic load (dst=pfx)",
	"cbgp_valid_net_traffic_load_dst_pfx"];

# -----[ cbgp_valid_net_traffic_load_dst_pfx ]-----------------------
# Check the ability to load a traffic matrix from a file and to load
# the links traversed by traffic. In this test, the traffic matrix
# specifies the flow destination with an IP prefix i.e. with fields
# 'dstIP' and 'dstMask'.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.1)
#   - N1 (192.168.0.0/30)
#   - N1 (192.168.0.4/30)
#   - Flow from R1 to 255/8: 100

#
# Topology:
#
#       *--{N1}--*
#   .1 /          \ .2
#     R1          R2
#   .5 \          / .6
#       *--{N2}--*
#
# Scenario:
#   - Add a static route towards 255.0.0/24 through N1
#   - Add a static route towards 255.0.0.1/32 through N2
#   - Load a traffic matrix and route based on IP address
#   - Check that link through N2 has load 100
#   - Check that link through N1 has load 0
#   - Load a traffic matrix and route based on IP prefix
#   - Check that link through N2 has load 100
#   - Check that link through N1 has load 100
# -------------------------------------------------------------------
sub cbgp_valid_net_traffic_load_dst_pfx($) {
  my ($cbgp)= @_;

  my $filename= get_resource("valid-net-tm-dst-pfx.tm");
  (! -e $filename) and return TEST_MISSING;

  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add link-ptp 1.0.0.1 192.168.0.1/30 1.0.0.2 192.168.0.2/30");
  $cbgp->send_cmd("net add link-ptp 1.0.0.1 192.168.0.5/30 1.0.0.2 192.168.0.6/30");
  $cbgp->send_cmd("net node 1.0.0.1 route add 255.0.0/24 --oif=192.168.0.1/30 1");
  $cbgp->send_cmd("net node 1.0.0.1 route add 255.0.0.1/32 --oif=192.168.0.5/30 1");

  my $msg= cbgp_check_error($cbgp, "net traffic load \"$filename\"");
  return TEST_FAILURE
    if (check_has_error($msg));

  my $info= cbgp_link_info($cbgp, '1.0.0.1', '192.168.0.5/30');
  return TEST_FAILURE
    if (!check_link_info($info, -load=>100));
  $info= cbgp_link_info($cbgp, '1.0.0.1', '192.168.0.1/30');
  return TEST_FAILURE
    if (!check_link_info($info, -load=>0));

  $msg= cbgp_check_error($cbgp, "net traffic load --dst=pfx \"$filename\"");
  return TEST_FAILURE
    if (check_has_error($msg));

  $info= cbgp_link_info($cbgp, '1.0.0.1', '192.168.0.5/30');
  return TEST_FAILURE
    if (!check_link_info($info, -load=>100));
  $info= cbgp_link_info($cbgp, '1.0.0.1', '192.168.0.1/30');
  return TEST_FAILURE
    if (!check_link_info($info, -load=>100));

  return TEST_SUCCESS;
}
