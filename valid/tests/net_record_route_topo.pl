return ["net record-route delay (topo)", "cbgp_valid_net_record_route_delay_topo", $topo];

# -----[ cbgp_valid_net_record_route_delay_topo ]--------------------
# Create the given topology in C-BGP. Check that record-route
# correctly records the path's IGP weight and delay.
# -------------------------------------------------------------------
sub cbgp_valid_net_record_route_delay_topo($$) {
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    if (!cbgp_topo_check_record_route($cbgp, $topo, "SUCCESS", 1)) {
      return TEST_FAILURE;
    }
    return TEST_SUCCESS;
  }

