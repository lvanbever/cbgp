return ["net igp overflow", "cbgp_valid_net_igp_overflow"];

# -----[ cbgp_valid_net_igp_overflow ]-------------------------------
# Create a small topology with huge weights. Check that the IGP
# computation will converge. Actually, if it does not converge, the
# validation process will be blocked...
# -------------------------------------------------------------------
sub cbgp_valid_net_igp_overflow($$)
  {
    my ($cbgp, $topo)= @_;
    my $rt;

    $cbgp->send_cmd("net add domain 1 igp");
    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 1.0.0.3");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");
    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 4000000000");
    $cbgp->send_cmd("net add link 1.0.0.2 1.0.0.3");
    $cbgp->send_cmd("net link 1.0.0.2 1.0.0.3 igp-weight --bidir 4000000000");
    $cbgp->send_cmd("net node 1.0.0.1 domain 1");
    $cbgp->send_cmd("net node 1.0.0.2 domain 1");
    $cbgp->send_cmd("net node 1.0.0.3 domain 1");
    $cbgp->send_cmd("net domain 1 compute");
    $rt= cbgp_get_rt($cbgp, "1.0.0.1", undef);
    return TEST_FAILURE
      if (!check_has_route($rt, '1.0.0.2/32', -metric=>4000000000));
    return TEST_FAILURE
      if (check_has_route($rt, '1.0.0.3/32'));
    return TEST_SUCCESS;
  }

