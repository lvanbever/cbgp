return ["net create", "cbgp_valid_net_create", $topo];

# -----[ cbgp_valid_net_create ]-------------------------------------
# Create the topology given as parameter in C-BGP, then checks that
# the topology is correctly setup.
# -------------------------------------------------------------------
sub cbgp_valid_net_create($$)
  {
    my ($cbgp, $topo)= @_;
    cbgp_topo($cbgp, $topo);
    return cbgp_topo_check_links($cbgp, $topo);
  }

