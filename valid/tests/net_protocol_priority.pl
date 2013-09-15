return ["net protocol priority", "cbgp_valid_net_protocol_priority"];

# -----[ cbgp_valid_net_protocol_priority ]--------------------------
# -------------------------------------------------------------------
sub cbgp_valid_net_protocol_priority($) {
  my ($cbgp)= @_;
  my $topo= topo_3nodes_line();
  cbgp_topo($cbgp, $topo, 1);
  cbgp_topo_igp_compute($cbgp, $topo, 1);
  $cbgp->send_cmd("net node 1.0.0.2 route add --oif=1.0.0.3 1.0.0.1/32 10");
  # Test protocol-priority in show-rt
  my $rt= cbgp_get_rt($cbgp, "1.0.0.2", "1.0.0.1");
  return TEST_FAILURE
    if (!check_has_route($rt, "1.0.0.1/32",
			 -proto=>C_RT_PROTO_STATIC,
			 -iface=>"1.0.0.3"));
  $rt= cbgp_get_rt($cbgp, "1.0.0.2", "1.0.0.3");
  return TEST_FAILURE
    if (!check_has_route($rt, "1.0.0.3/32",
			 -proto=>C_RT_PROTO_IGP,
			 -iface=>"1.0.0.3"));
  # Test protocol-priority in record-route
  my $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.0.0.1");
  return TEST_FAILURE
    if (!check_recordroute($trace, -path=>[[1,"1.0.0.3"]]));
  $trace= cbgp_record_route($cbgp, "1.0.0.2", "1.0.0.3");
  return TEST_FAILURE
    if (!check_recordroute($trace, -path=>[[1,"1.0.0.3"]]));
  return TEST_SUCCESS;
}

