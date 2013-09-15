return ["net record-route ecmp (load)", "cbgp_valid_net_record_route_ecmp_load"];

# -----[ cbgp_valid_net_record_route_ecmp_load ]---------------------
# Test that record-route enumerates all possible paths, and that the
# load is correctly splitted among the various paths.
#
# Setup:
#   - R1 (1.0.0.1, AS1)
#   - R2 (1.0.0.2, AS1)
#   - R3 (1.0.0.3, AS1)
#   - R4 (1.0.0.4, AS1)
#   - IGP weight equals 1 on all links
#
# Topology:
#
#     *-- R2 --*
#    /          \
#   R1          R4
#    \          /
#     *-- R3 --*
#
# Scenario:
#   * Record route from node 1.0.0.1 towards 1.0.0.4 w/ ECMP
#     and loads 1000 units of traffic
#   * Check that the load on every link is 500
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_ecmp_load($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.4");
  $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4");
  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net domain 1 compute");
  my $traces= cbgp_record_route($cbgp, "1.0.0.1", "1.0.0.4",
				-ecmp=>1, -load=>1000);
  return TEST_FAILURE
    if (scalar(@$traces) != 2);
  my $info= cbgp_link_info($cbgp, "1.0.0.1", "1.0.0.2");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>500)); 
  $info= cbgp_link_info($cbgp, "1.0.0.1", "1.0.0.3");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>500));
  $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.4");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>500));
  $info= cbgp_link_info($cbgp, "1.0.0.2", "1.0.0.4");
  return TEST_FAILURE
    if (!check_link_info($info, -load=>500));
  return TEST_SUCCESS;
}

