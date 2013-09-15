return ["net record-route load", "cbgp_valid_net_record_route_load"];

# -----[ cbgp_valid_net_record_route_load ]--------------------------
# Check that record-route is able to load the links along a path
# with the given volume of traffic.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#   - R5 (1.0.0.5)
#   - R6 (1.0.0.6)
#
# Topology:
#
#   R1 --(1)--*             *--(1)-- R5
#              \           /
#     *--(2)-- R2 --(1)-- R4 --(2)--*
#    /                               \
#   R3 -------------(2)------------- R6
#
#   (W) represent the link's IGP weight
#
# Scenario:
#   * Load R1 -> R5 with 1000
#   * Load R1 -> R6 with 500
#   * Load R1 -> R4 with 250
#   * Load R3 -> R5 with 125
#   * Load R3 -> R6 with 1000
#   * Check that capacity(R1->R2)=1750
#   * Check that capacity(R2->R4)=1875
#   * Check that capacity(R3->R2)=125
#   * Check that capacity(R3->R6)=1000
#   * Check that capacity(R4->R5)=1125
#   * Check that capacity(R4->R6)=500
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_load($) {
  my ($cbgp)= @_;
  my $info;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.5");
  $cbgp->send_cmd("net node 1.0.0.5 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.6");
  $cbgp->send_cmd("net node 1.0.0.6 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 2");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.6");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.6 igp-weight --bidir 2");
  $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.5");
  $cbgp->send_cmd("net link 1.0.0.4 1.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net add link 1.0.0.4 1.0.0.6");
  $cbgp->send_cmd("net link 1.0.0.4 1.0.0.6 igp-weight --bidir 2");
  $cbgp->send_cmd("net domain 1 compute");

  cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.5", -load=>1000);
  cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.6", -load=>500);
  cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.4", -load=>250);
  cbgp_record_route($cbgp, "1.0.0.3", "1.0.0.5", -load=>125);
  cbgp_record_route($cbgp, "1.0.0.3", "1.0.0.6", -load=>1000);

  $info= cbgp_link_info($cbgp, "1.0.0.1", "1.0.0.2");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>1750));
  if (!exists($info->{load}) || ($info->{load} != 1750)) {
    $tests->debug("Error: load of R1->R2 should be 1750");
    return TEST_FAILURE;
  }
  $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.4");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>1875));
  $info= cbgp_link_info($cbgp, "1.0.0.3", "1.0.0.2");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>125));
  $info= cbgp_link_info($cbgp, "1.0.0.3", "1.0.0.6");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>1000));
  $info= cbgp_link_info($cbgp, "1.0.0.4", "1.0.0.5");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>1125));
  $info= cbgp_link_info($cbgp, "1.0.0.4", "1.0.0.6");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>500));
  return TEST_SUCCESS;
}

