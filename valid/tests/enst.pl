return ["enst", "cbgp_valid_enst"];

# -----[ cbgp_valid_enst ]-------------------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_enst($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add node 137.194.1.1");
  $cbgp->send_cmd("net add node 212.27.32.1");
  $cbgp->send_cmd("net add node 134.157.1.1");
  $cbgp->send_cmd("net add node 4.1.1.1");
  $cbgp->send_cmd("net add node 62.229.1.1");

  $cbgp->send_cmd("net add link 137.194.1.1 212.27.32.1");
  $cbgp->send_cmd("net add link 137.194.1.1 134.157.1.1");
  $cbgp->send_cmd("net add link 212.27.32.1 4.1.1.1");
  $cbgp->send_cmd("net add link 62.229.1.1 134.157.1.1");
  $cbgp->send_cmd("net add link 62.229.1.1 4.1.1.1");

  $cbgp->send_cmd("net node 137.194.1.1 route add --gw=134.157.1.1 134.157.0.0/14 5");
  $cbgp->send_cmd("net node 137.194.1.1 route add --gw=212.27.32.1 212.27.32.0/19 5");
  $cbgp->send_cmd("net node 212.27.32.1 route add --gw=4.1.1.1 4.0.0.0/8 5");
  $cbgp->send_cmd("net node 212.27.32.1 route add --gw=137.194.1.1 137.194.0.0/16 5");
  $cbgp->send_cmd("net node 4.1.1.1 route add --gw=212.27.32.1 212.27.32.0/19 5");
  $cbgp->send_cmd("net node 134.157.1.1 route add --gw=137.194.1.1 137.194.0.0/16 5");
  $cbgp->send_cmd("net node 62.229.1.1 route add --gw=134.157.1.1 134.157.0.0/14 5");
  $cbgp->send_cmd("net node 62.229.1.1 route add --gw=4.1.1.1 4.0.0.0/8 5");
  $cbgp->send_cmd("net node 134.157.1.1 route add --gw=62.229.1.1 62.229.0.0/16 5");
  $cbgp->send_cmd("net node 4.1.1.1 route add --gw=62.229.1.1 62.229.0.0/16 5");

  $cbgp->send_cmd("bgp add router 1712 137.194.1.1");
  $cbgp->send_cmd("bgp add router 12322 212.27.32.1");
  $cbgp->send_cmd("bgp add router 2200 134.157.1.1");
  $cbgp->send_cmd("bgp add router 3356 4.1.1.1");
  $cbgp->send_cmd("bgp add router 5511 62.229.1.1");

  $cbgp->send_cmd("bgp router 62.229.1.1");
  $cbgp->send_cmd("\tadd network 62.229.0.0/16");
  $cbgp->send_cmd("\tadd peer 3356 4.1.1.1");
  $cbgp->send_cmd("\tadd peer 2200 134.157.1.1");
  $cbgp->send_cmd("\tpeer 4.1.1.1 up");
  $cbgp->send_cmd("\tpeer 134.157.1.1 up");
  $cbgp->send_cmd("\texit");
  $cbgp->send_cmd("bgp router 137.194.1.1");
  $cbgp->send_cmd("\tadd network 137.194.0.0/16");
  $cbgp->send_cmd("\tadd peer 12322 212.27.32.1");
  $cbgp->send_cmd("\tadd peer 2200 134.157.1.1");
  $cbgp->send_cmd("\tpeer 212.27.32.1 up");
  $cbgp->send_cmd("\tpeer 134.157.1.1 up");
  $cbgp->send_cmd("\tpeer 212.27.32.1");
  $cbgp->send_cmd("\t\tfilter in");
  $cbgp->send_cmd("\t\t\tadd-rule");
  $cbgp->send_cmd("\t\t\t\tmatch any");
  $cbgp->send_cmd("\t\t\t\taction \"community add 1\"");
  $cbgp->send_cmd("\t\t\t\texit");
  $cbgp->send_cmd("\t\t\texit");
  $cbgp->send_cmd("\t\tfilter out");
  $cbgp->send_cmd("\t\t\tadd-rule");
  $cbgp->send_cmd("\t\t\t\tmatch \"community is 1\"");
  $cbgp->send_cmd("\t\t\t\taction deny");
  $cbgp->send_cmd("\t\t\t\texit");
  $cbgp->send_cmd("\t\t\texit");
  $cbgp->send_cmd("\t\texit");
  $cbgp->send_cmd("\tpeer 134.157.1.1");
  $cbgp->send_cmd("\t\tfilter in");
  $cbgp->send_cmd("\t\t\tadd-rule");
  $cbgp->send_cmd("\t\t\t\tmatch any");
  $cbgp->send_cmd("\t\t\t\taction \"community add 1\"");
  $cbgp->send_cmd("\t\t\t\texit");
  $cbgp->send_cmd("\t\t\texit");
  $cbgp->send_cmd("\t\tfilter out");
  $cbgp->send_cmd("\t\t\tadd-rule");
  $cbgp->send_cmd("\t\t\t\tmatch \"community is 1\"");
  $cbgp->send_cmd("\t\t\t\taction deny");
  $cbgp->send_cmd("\t\t\t\texit");
  $cbgp->send_cmd("\t\t\texit");
  $cbgp->send_cmd("\t\texit");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp router 212.27.32.1");
  $cbgp->send_cmd("\tadd network 212.27.32.0/19");
  $cbgp->send_cmd("\tadd network 88.160.0.0/11");
  $cbgp->send_cmd("\tadd peer 1712 137.194.1.1");
  $cbgp->send_cmd("\tadd peer 3356 4.1.1.1");
  $cbgp->send_cmd("\tpeer 137.194.1.1 up");
  $cbgp->send_cmd("\tpeer 4.1.1.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp router 134.157.1.1");
  $cbgp->send_cmd("\tadd network 134.157.0.0/16");
  $cbgp->send_cmd("\tadd peer 5511 62.229.1.1");
  $cbgp->send_cmd("\tadd peer 1712 137.194.1.1");
  $cbgp->send_cmd("\tpeer 62.229.1.1 up");
  $cbgp->send_cmd("\tpeer 137.194.1.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp router 4.1.1.1");
  $cbgp->send_cmd("\tadd network 4.0.0.0/8");
  $cbgp->send_cmd("\tadd network 212.73.192.0/18");
  $cbgp->send_cmd("\tadd peer 12322 212.27.32.1");
  $cbgp->send_cmd("\tadd peer 5511 62.229.1.1");
  $cbgp->send_cmd("\tpeer 212.27.32.1 up");
  $cbgp->send_cmd("\tpeer 62.229.1.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("sim run");
  return TEST_SUCCESS;
}

