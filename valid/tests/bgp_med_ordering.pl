return ["nasty: bgp med ordering",
	"cbgp_valid_nasty_bgp_med_ordering"];

# -----[ cbgp_valid_nasty_bgp_med_ordering ]-------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_nasty_bgp_med_ordering($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.4");
  $cbgp->send_cmd("net node 1.0.0.4 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 20");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 5");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.4");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.4 igp-weight --bidir 10");
  $cbgp->send_cmd("net domain 1 compute");

  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("\tadd peer 2 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\tadd peer 2 1.0.0.3");
  $cbgp->send_cmd("\tpeer 1.0.0.3 up");
  $cbgp->send_cmd("\tadd peer 3 1.0.0.4");
  $cbgp->send_cmd("\tpeer 1.0.0.4 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 2 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tadd network 255/8");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1");
  $cbgp->send_cmd("\t\tfilter out");
  $cbgp->send_cmd("\t\t\tadd-rule");
  $cbgp->send_cmd("\t\t\t\tmatch any");
  $cbgp->send_cmd("\t\t\t\taction \"metric 10\"");
  $cbgp->send_cmd("\t\t\t\texit");
  $cbgp->send_cmd("\t\t\texit");
  $cbgp->send_cmd("\t\texit");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 2 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.3");
  $cbgp->send_cmd("\tadd network 255/8");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1");
  $cbgp->send_cmd("\t\tfilter out");
  $cbgp->send_cmd("\t\t\tadd-rule");
  $cbgp->send_cmd("\t\t\t\tmatch any");
  $cbgp->send_cmd("\t\t\t\taction \"metric 20\"");
  $cbgp->send_cmd("\t\t\t\texit");
  $cbgp->send_cmd("\t\t\texit");
  $cbgp->send_cmd("\t\texit");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 3 1.0.0.4");
  $cbgp->send_cmd("bgp router 1.0.0.4");
  $cbgp->send_cmd("\tadd network 255/8");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("sim run");

  my $rib= cbgp_get_rib($cbgp, '1.0.0.1', '255/8');
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>'1.0.0.4'));

  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 down");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp router 1.0.0.1 rescan");
  $cbgp->send_cmd("bgp router 1.0.0.2 rescan");
  $cbgp->send_cmd("sim run");

  $rib= cbgp_get_rib($cbgp, '1.0.0.1', '255/8');
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>'1.0.0.3'));

  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 up");
  $cbgp->send_cmd("net domain 1 compute");
  $cbgp->send_cmd("bgp router 1.0.0.1 rescan");
  $cbgp->send_cmd("bgp router 1.0.0.2 rescan");
  $cbgp->send_cmd("sim run");

  $rib= cbgp_get_rib($cbgp, '1.0.0.1', '255/8');
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>'1.0.0.4'));

  $cbgp->send_cmd("bgp router 1.0.0.2 del network 255/8");
  $cbgp->send_cmd("sim run");

  $rib= cbgp_get_rib($cbgp, '1.0.0.1', '255/8');
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "255/8",
			     -nexthop=>'1.0.0.3'));
  return TEST_SUCCESS;
}

