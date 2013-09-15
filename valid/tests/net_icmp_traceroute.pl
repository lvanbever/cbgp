return ["net icmp traceroute", "cbgp_valid_net_icmp_traceroute"];

# -----[ cbgp_valid_net_icmp_traceroute ]---------------------------
# Test the behaviour of the traceroute command.
# ------------------------------------------------------------------
sub cbgp_valid_net_icmp_traceroute($)
  {
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
    $cbgp->send_cmd("net add subnet 192.168.0/24 transit");
    $cbgp->send_cmd("net add subnet 192.168.1/24 transit");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.1 192.168.1.1/24");
    $cbgp->send_cmd("net link 1.0.0.1 192.168.1.1/24 igp-weight 10");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 10");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.1/24");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.1/24 igp-weight 10");
    $cbgp->send_cmd("net add link 1.0.0.3 1.0.0.4");
    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 igp-weight --bidir 10");
    $cbgp->send_cmd("net domain 1 compute");

    $cbgp->send_cmd("net link 1.0.0.3 1.0.0.4 down");
    $cbgp->send_cmd("net node 1.0.0.1 route add --oif=1.0.0.2 1.0.0.5/32 1");
    $cbgp->send_cmd("net node 1.0.0.1 route add --oif=1.0.0.2 1.0.0.6/32 1");
    $cbgp->send_cmd("net node 1.0.0.2 route add --oif=192.168.0.1/24 1.0.0.6/32 1");
    $cbgp->send_cmd("net node 1.0.0.1 route add --oif=192.168.1.1/24 1.0.0.8/32 1");

    my $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.1');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>1,
			    -hops=>[[0,'1.0.0.1',C_PING_STATUS_REPLY]]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.2');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>1,
			    -hops=>[[0,'1.0.0.2',C_PING_STATUS_REPLY]]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.3');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>2,
			    -hops=>[[1,'1.0.0.3',C_PING_STATUS_REPLY]]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.4');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>3,
			    -hops=>[[2,'*',C_PING_STATUS_NO_REPLY]]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.5');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>2,
			    -hops=>[[1,'1.0.0.2',C_PING_STATUS_ICMP_HOST]]));
#    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.6');
#    return TEST_FAILURE
#      if (!check_traceroute($tr, -nhops=>2,
#			    -hops=>[[1,'1.0.0.2',C_PING_STATUS_ICMP_HOST]]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.7');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>1,
			    -hops=>[[0,'*',C_PING_STATUS_HOST]]));
    $tr= cbgp_traceroute($cbgp, '1.0.0.1', '1.0.0.8');
    return TEST_FAILURE
      if (!check_traceroute($tr, -nhops=>1,
			    -hops=>[[0,'*',C_PING_STATUS_HOST]]));
    return TEST_SUCCESS;
  }

