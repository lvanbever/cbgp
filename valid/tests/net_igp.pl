return ["net igp", "cbgp_valid_net_igp", $topo];

# -----[ cbgp_valid_net_igp ]----------------------------------------
# Create the topology given as parameter in C-BGP and compute the
# shortest paths. Finally checks that each node can reach the other
# ones.
# -------------------------------------------------------------------
sub cbgp_valid_net_igp($$)
  {
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo, 1);
    cbgp_topo_igp_compute($cbgp, $topo, 1);
    return cbgp_topo_check_igp_reachability($cbgp, $topo);
  }

