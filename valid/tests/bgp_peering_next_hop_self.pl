return ["bgp peering next-hop-self",
	"cbgp_valid_bgp_peering_nexthopself"];

# -----[ cbgp_valid_bgp_peering_nexthopself ]-----------------------
# Test ability for a BGP router to set the BGP next-hop of a route to
# its own address before propagating it to a given neighbor. This test
# uses the 'next-hop-self' peer option.
#
# Setup:
#   - R1 (1.0.0.1, AS1, IGP domain 1)
#   - R2 (1.0.0.2, AS1, IGP domain 1)
#   - R3 (2.0.0.1, AS2, IGP domain 2)
#   - static routes between R1 and R2
#
# Topology:
#
#   AS2 \    / AS1
#        |  |
#   R3 --]--[-- R1 ----- R2
#        |  | nhs
#       /    \
#
# Scenario:
#   * R3 originates prefix 2/8
#   * R1 propagates 2/8 to R2
#   * Check that R2 has received the route towards 2/8 with
#     next-hop=R1
# -------------------------------------------------------------------
sub cbgp_valid_bgp_peering_nexthopself($) {
  my ($cbgp)= @_;
  my $topo= topo_2nodes();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  cbgp_topo_bgp_routers($cbgp, $topo, 1);
  $cbgp->send_cmd("bgp domain 1 full-mesh");
  $cbgp->send_cmd("net add node 2.0.0.1");
  $cbgp->send_cmd("net add link 1.0.0.1 2.0.0.1");
  $cbgp->send_cmd("net node 1.0.0.1 route add --oif=2.0.0.1 2.0.0.1/32 0");
  $cbgp->send_cmd("net node 2.0.0.1 route add --oif=1.0.0.1 1.0.0.1/32 0");
  $cbgp->send_cmd("bgp add router 2 2.0.0.1");
  $cbgp->send_cmd("bgp router 2.0.0.1 add network 2.0.0.0/8");
  cbgp_peering($cbgp, "2.0.0.1", "1.0.0.1", 1);
  cbgp_peering($cbgp, "1.0.0.1", "2.0.0.1", 2, "next-hop-self");
  $cbgp->send_cmd("sim run");
  # Next-hop must be unchanged in 1.0.0.1
  my $rib= cbgp_get_rib($cbgp, "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "2/8",
			     -nexthop=>"2.0.0.1"));

  # Next-hop must be updated with address of 1.0.0.1 in 1.0.0.2
  $rib= cbgp_get_rib($cbgp, "1.0.0.2");
  return TEST_FAILURE
    if (!check_has_bgp_route($rib, "2/8",
			     -nexthop=>"1.0.0.1"));

  return TEST_SUCCESS;
}

