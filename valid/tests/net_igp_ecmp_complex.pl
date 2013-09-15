use strict;

return ["net igp ecmp (complex)", "cbgp_valid_net_igp_ecmp_complex"];

# -----[ cbgp_valid_net_igp_ecmp_complex ]---------------------------
# Test the ECMP capability of the IGP model on a complex topology.
#
# Setup:
#   - R1 (0.0.0.1)
#   - R2 (0.0.0.2)
#   - R3 (0.0.0.3)
#   - R4 (0.0.0.4)
#   - R5 (0.0.0.5)
#   - R6 (0.0.0.6)
#   - R7 (0.0.0.7)
#   - R8 (0.0.0.8)
#   - R9 (0.0.0.9)
#   - all links have an IGP weight equal to 1 except R4-R7 where it is 2
#
# Topology:
#
#            R3 --- R5 -- R6
#           /     /   \     \
#   R1 -- R2     /     \     R8 -- R9
#           \   /       \   /
#            R4 ---(2)--- R7
#
# Scenario:
#   * this setup should provide 5 different paths from R1 to R9
#     (R1, R2, R3, R5, R6, R8, R9)
#     (R1, R2, R3, R5, R7, R8, R9)
#     (R1, R2, R4, R5, R6, R8, R9)
#     (R1, R2, R4, R5, R7, R8, R9)
#     (R1, R2, R4, R7, R8, R8)
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_ecmp_complex($) {
  my ($cbgp)= @_;
  $cbgp->send_cmd("net add node 0.0.0.1");
  $cbgp->send_cmd("net add node 0.0.0.2");
  $cbgp->send_cmd("net add node 0.0.0.3");
  $cbgp->send_cmd("net add node 0.0.0.4");
  $cbgp->send_cmd("net add node 0.0.0.5");
  $cbgp->send_cmd("net add node 0.0.0.6");
  $cbgp->send_cmd("net add node 0.0.0.7");
  $cbgp->send_cmd("net add node 0.0.0.8");
  $cbgp->send_cmd("net add node 0.0.0.9");
  $cbgp->send_cmd("net add link 0.0.0.1 0.0.0.2");
  $cbgp->send_cmd("net add link 0.0.0.2 0.0.0.3");
  $cbgp->send_cmd("net add link 0.0.0.2 0.0.0.4");
  $cbgp->send_cmd("net add link 0.0.0.3 0.0.0.5");
  $cbgp->send_cmd("net add link 0.0.0.4 0.0.0.5");
  $cbgp->send_cmd("net add link 0.0.0.5 0.0.0.6");
  $cbgp->send_cmd("net add link 0.0.0.5 0.0.0.7");
  $cbgp->send_cmd("net add link 0.0.0.6 0.0.0.8");
  $cbgp->send_cmd("net add link 0.0.0.7 0.0.0.8");
  $cbgp->send_cmd("net add link 0.0.0.8 0.0.0.9");
  $cbgp->send_cmd("net add link 0.0.0.4 0.0.0.7");

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net node 0.0.0.1 domain 1");
  $cbgp->send_cmd("net node 0.0.0.2 domain 1");
  $cbgp->send_cmd("net node 0.0.0.3 domain 1");
  $cbgp->send_cmd("net node 0.0.0.4 domain 1");
  $cbgp->send_cmd("net node 0.0.0.5 domain 1");
  $cbgp->send_cmd("net node 0.0.0.6 domain 1");
  $cbgp->send_cmd("net node 0.0.0.7 domain 1");
  $cbgp->send_cmd("net node 0.0.0.8 domain 1");
  $cbgp->send_cmd("net node 0.0.0.9 domain 1");
  $cbgp->send_cmd("net link 0.0.0.1 0.0.0.2 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.2 0.0.0.3 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.2 0.0.0.4 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.3 0.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.4 0.0.0.5 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.5 0.0.0.6 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.5 0.0.0.7 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.6 0.0.0.8 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.7 0.0.0.8 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.8 0.0.0.9 igp-weight --bidir 1");
  $cbgp->send_cmd("net link 0.0.0.4 0.0.0.7 igp-weight --bidir 2");
  $cbgp->send_cmd("net domain 1 compute");

  my $filename= get_tmp_resource("cbgp-topology-ecmp.dot");
  $cbgp->send_cmd("net export --format=dot --output=$filename");

  my $traces= cbgp_record_route($cbgp, '0.0.0.1', '0.0.0.9', -ecmp=>1);
  return TEST_FAILURE
    if (scalar(@$traces) != 5);

  foreach my $trace (@$traces) {
  }

  return TEST_SUCCESS;
}
