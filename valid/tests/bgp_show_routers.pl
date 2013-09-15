return ["bgp show routers", "cbgp_valid_bgp_show_routers"];

# -----[ cbgp_valid_bgp_show_routers ]-------------------------------
# Test the ability to list all the BGP routers defined in the
# simulation.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_show_routers($)
  {
    my ($cbgp)= @_;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add node 2.0.0.1");
    $cbgp->send_cmd("net add node 2.0.0.2");
    $cbgp->send_cmd("net add node 2.0.0.3");

    $cbgp->send_cmd("bgp add router 1 1.0.0.1");
    $cbgp->send_cmd("bgp add router 1 1.0.0.2");
    $cbgp->send_cmd("bgp add router 2 2.0.0.1");
    $cbgp->send_cmd("bgp add router 2 2.0.0.2");
    $cbgp->send_cmd("bgp add router 2 2.0.0.3");

    my $rtrs= cbgp_get_routers($cbgp);
    my @routers= sort (keys %$rtrs);
    if ((scalar(@routers) != 5) || ($routers[0] ne '1.0.0.1') ||
	($routers[1] ne '1.0.0.2') || ($routers[2] ne '2.0.0.1') ||
	($routers[3] ne '2.0.0.2') || ($routers[4] ne '2.0.0.3')) {
      return TEST_FAILURE;
    }

    $rtrs= cbgp_get_routers($cbgp, "1");
    @routers= sort (keys %$rtrs);
    if ((scalar(@routers) != 2) || ($routers[0] ne '1.0.0.1') ||
	($routers[1] ne '1.0.0.2')) {
      return TEST_FAILURE;
    }

    $rtrs= cbgp_get_routers($cbgp, "2.0.0.2");
    @routers= sort (keys %$rtrs);
    if ((scalar(@routers) != 1) || ($routers[0] ne '2.0.0.2')) {
      return TEST_FAILURE;
    }

    return TEST_SUCCESS;
  }

