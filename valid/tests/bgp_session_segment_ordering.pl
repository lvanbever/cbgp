return ["nasty: bgp session segment ordering",
	"cbgp_valid_nasty_bgp_session_segment_ordering"];

# -----[ cbgp_valid_nasty_bgp_session_segment_ordering ]-------------
#
# -------------------------------------------------------------------
sub cbgp_valid_nasty_bgp_session_segment_ordering($) {
  my ($cbgp)= @_;

  $cbgp->send_cmd("net add domain 1 igp");
  $cbgp->send_cmd("net add node 1.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.2");
  $cbgp->send_cmd("net node 1.0.0.2 domain 1");
  $cbgp->send_cmd("net add node 1.0.0.3");
  $cbgp->send_cmd("net node 1.0.0.3 domain 1");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 2");
  $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.1 1.0.0.3 igp-weight --bidir 10");
  $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
  $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
  $cbgp->send_cmd("net domain 1 compute");

  $cbgp->send_cmd("bgp add router 1 1.0.0.1");
  $cbgp->send_cmd("bgp router 1.0.0.1");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.2");
  $cbgp->send_cmd("\tpeer 1.0.0.2 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp add router 1 1.0.0.2");
  $cbgp->send_cmd("bgp router 1.0.0.2");
  $cbgp->send_cmd("\tadd peer 1 1.0.0.1");
  $cbgp->send_cmd("\tpeer 1.0.0.1 up");
  $cbgp->send_cmd("\texit");

  $cbgp->send_cmd("bgp router 1.0.0.1 add network 254/8");
  $cbgp->send_cmd("sim run");

  $cbgp->send_cmd("net node 1.0.0.1 route add --oif=1.0.0.3 1.0.0.2/32 1");

  $cbgp->send_cmd("bgp router 1.0.0.1 del network 254/8");
  $cbgp->send_cmd("net node 1.0.0.1 route del 1.0.0.2/32 1.0.0.3");
  $cbgp->send_cmd("bgp router 1.0.0.1 add network 254/8");

  $reason= cbgp_check_error($cbgp, "sim run");
	print "[[$reason]]\n";
  return TEST_FAILURE
    if !check_has_error($reason, "BGP message received out-of-sequence");

  return TEST_SUCCESS;
}

