return ["net record-route prefix", "cbgp_valid_net_record_route_prefix"];

# -----[ cbgp_valid_net_record_route_prefix ]-----------------------
# Test that record-route works with a prefix destination.
# ------------------------------------------------------------------
sub cbgp_valid_net_record_route_prefix($)
  {
    my ($cbgp)= @_;
    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add subnet 192.168.0/24 stub");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 1");
    $cbgp->send_cmd("net add link 1.0.0.2 192.168.0.2/24");
    $cbgp->send_cmd("net link 1.0.0.2 192.168.0.2/24 igp-weight 1");
    $cbgp->send_cmd("net domain 1 compute");

    my $trace= cbgp_record_route($cbgp, "1.0.0.1", "192.168.0/24");
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"SUCCESS"));
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "192.168.0.24");
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"UNREACH"));
    $trace= cbgp_record_route($cbgp, "1.0.0.1", "192.168.0.2");
    return TEST_FAILURE
      if (!check_recordroute($trace, -status=>"SUCCESS"));
    return TEST_SUCCESS;
  }

