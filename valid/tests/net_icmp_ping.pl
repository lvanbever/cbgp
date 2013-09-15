return ["net icmp ping", "cbgp_valid_net_icmp_ping"];

# -----[ cbgp_valid_net_icmp_ping ]---------------------------------
# Test the behaviour of the ping command.
#
# Setup:
#   - R1 (1.0.0.1)
#   - R2 (1.0.0.2)
#   - R3 (1.0.0.3)
#   - R4 (1.0.0.4)
#   - [R5 (1.0.0.5)]
#   - [R6 (1.0.0.6)]
#   - [R7 (1.0.0.7)]
#   - [R8 (1.0.0.8)]
#   - N1 (192.168.0/24)
#   - N2 (192.168.1/24)
#
# Topology:
#
#   R1 ----- R2 ----- R3 --x-- R4
#    |        |
#   (N2)     (N1)
#
# Scenario:
#   * Compute IGP routes
#   * Break link R3-R4
#   * Add to R1 a static route towards R5 through N1 (obviously incorrect)
#   * Add to R1 a static route towards R6 through R2 (obviously incorrect)
#   * Add to R1 a static route towards R8 through N2 (obviously incorrect)
#   * Ping R1: should succeed
#   * Ping R2: should succeed
#   * Ping R3: should succeed
#   * Ping R3 with TTL=1: should fail with icmp-time-exceeded
#   * Ping R4: should fail with no-reply
#   * Ping R5: should fail with icmp-net-unreach
#   * Ping R6: should fail with icmp-dst-unreach
#   * Ping R7: should wail with net-unreach
#   * Ping R8: should fail with dst-unreach
# ------------------------------------------------------------------
sub cbgp_valid_net_icmp_ping($)
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

    my $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.1');
    return TEST_FAILURE
      if (!check_ping($ping, -status=>C_PING_STATUS_REPLY,
		      -addr=>'1.0.0.1'));
    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.2');
    return TEST_FAILURE
      if (!check_ping($ping, -status=>C_PING_STATUS_REPLY,
		      -addr=>'1.0.0.2'));
    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.3');
    return TEST_FAILURE
      if (!check_ping($ping, -status=>C_PING_STATUS_REPLY,
		      -addr=>'1.0.0.3'));
    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.3', -ttl=>1);
    return TEST_FAILURE
      if (!check_ping($ping, -status=>C_PING_STATUS_ICMP_TTL,
		      -addr=>'1.0.0.2'));
    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.4');
    return TEST_FAILURE
      if (!check_ping($ping, -status=>C_PING_STATUS_NO_REPLY));
    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.5');
    return TEST_FAILURE
      if (!check_ping($ping, -status=>C_PING_STATUS_ICMP_HOST,
		      -addr=>'1.0.0.2'));
#    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.6');
#    return TEST_FAILURE
#      if (!check_ping($ping, -status=>C_PING_STATUS_ICMP_HOST,
#		     -addr=>'1.0.0.2'));
    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.7');
    return TEST_FAILURE
      if (!check_ping($ping, -status=>C_PING_STATUS_HOST));
    $ping= cbgp_ping($cbgp, '1.0.0.1', '1.0.0.8');
    return TEST_FAILURE
      if (!check_ping($ping, -status=>C_PING_STATUS_HOST));
    return TEST_SUCCESS;
  }

