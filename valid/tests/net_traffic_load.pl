use strict;

return ["net traffic load", "cbgp_valid_net_traffic_load"];

# -----[ cbgp_valid_net_traffic_load ]-------------------------------
# Check the ability to load a traffic matrix from a file and to load
# the links traversed by traffic.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#   - N1 (172.13.16/24)
#   - N2 (138.48/16)
#   - N3 (130.104/16)
#
# Topology:
#
#   R1 ----- R2 ----- R3 -----{N2}
#   |         \
#   |          *----- R4 -----{N1}
#  {N3}
#
# Scenario:
#   * Setup topology and load traffic matrix from file "valid-tm.txt"
#   * Check that load(R1->R1)=1500
#   * Check that load(R2->R3)=1800
#   * Check that load(R3->N2)=1600
#   * Check that load(R2->R4)=1200
#   * Check that load(R4->N1)=800
#   * Check that load(R1->N3)=3200
#   * Check that load(R2->R1)=3200
#
# Resources:
#   [valid-net-tm.tm]
# -------------------------------------------------------------------
sub cbgp_valid_net_traffic_load($) {
  my ($cbgp)= @_;
  my $msg;
  my $info;

  my $filename= get_resource("valid-net-tm.tm");
  (-e $filename) or return TEST_SKIPPED;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add subnet 172.13.16/24 stub");
  $cbgp->send_cmd("net add subnet 138.48/16 stub");
  $cbgp->send_cmd("net add subnet 130.104/16 stub");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 10");
  $cbgp->send_cmd("net add link 1.0.0.1 130.104.10.10/16");
  $cbgp->send_cmd("net link 1.0.0.1 130.104.10.10/16 igp-weight 10");
  $cbgp->send_cmd("net add link 1.0.0.3 138.48.4.4/16");
  $cbgp->send_cmd("net link 1.0.0.3 138.48.4.4/16 igp-weight 10");
  $cbgp->send_cmd("net add link 1.0.0.4 172.13.16.1/24");
  $cbgp->send_cmd("net link 1.0.0.4 172.13.16.1/24 igp-weight 10");
  $cbgp->send_cmd("net domain 1 compute");

  $msg= cbgp_check_error($cbgp, "net traffic load \"$filename\"");
  return TEST_FAILURE
    if (check_has_error($msg));

  $info= cbgp_link_info($cbgp, "1.0.0.1", "1.0.0.2");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>1500));

  $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.3");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>1800));

  $info= cbgp_link_info($cbgp, "1.0.0.3", "138.48.4.4/16");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>1600));

  $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.4");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>1200));

  $info= cbgp_link_info($cbgp, "1.0.0.4", "172.13.16.1/24");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>800));

  $info= cbgp_link_info($cbgp, "1.0.0.1", "130.104.10.10/16");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>3200));

  $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.1");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>3200));

  return TEST_SUCCESS;
}

