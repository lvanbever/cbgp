return ["bgp as-path as_set", "cbgp_valid_bgp_aspath_as_set"];

# -----[ cbgp_valid_bgp_aspath_as_set ]------------------------------
# Send an AS-Path with an AS_SET segment and checks that it is
# correctly handled.
# -------------------------------------------------------------------
sub cbgp_valid_bgp_aspath_as_set($) {
  #    my ($cbgp)= @_;
  #    my ($rib, $route);
  #    my $topo= topo_2nodes();
  #    cbgp_topo($cbgp, $topo, 1);
  #    cbgp_topo_igp_compute($cbgp, $topo, 1);
  #    cbgp_topo_bgp_routers($cbgp, $topo, 1);
  #    die if $cbgp->send("bgp domain 1 full-mesh\n");
  #    die if $cbgp->send("net add node 2.0.0.1\n");
  #    die if $cbgp->send("net add link 1.0.0.1 2.0.0.1\n");
  #    die if $cbgp->send("net node 1.0.0.1 route add 2.0.0.1/32 2.0.0.1 0\n");
  #    cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self", "virtual");
  #    die if $cbgp->send("sim run\n");
  #    die if $cbgp->send("bgp router 1.0.0.1 peer 2.0.0.1 recv ".
  #		       "\"BGP4|0|A|1.0.0.1|1|255.255.0.0/16|".
  #		       "2 3 [4 5 6]|IGP|2.0.0.1|12|34|\"\n");
  #    # Check that route is in the Loc-RIB of 1.0.0.1
  #    $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  #    (!exists($rib->{"255.255.0.0/16"})) and
  #	return TEST_FAILURE;
  #    $route= $rib->{"255.255.0.0/16"};
  #    print "AS-Path: ".$route->[F_RIB_PATH]."\n";
  #    ($route->[F_RIB_PATH] =~ m/^2 3 \[([456 ]+)\]$/) or
  #	return TEST_FAILURE;
  #    
  return TEST_TODO;
}

