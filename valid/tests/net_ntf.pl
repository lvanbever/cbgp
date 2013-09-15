return ["net ntf load", "cbgp_valid_net_ntf_load", get_resource("valid-record-route.ntf")];

# -----[ cbgp_valid_net_ntf_load ]-----------------------------------
# Load a topology from an NTF file into C-BGP (using C-BGP 's "net
# ntf load" command). Check that the links are correctly setup.
#
# Resources:
#   [valid-record-route.ntf]
# -------------------------------------------------------------------
sub cbgp_valid_net_ntf_load($$) {
  my ($cbgp, $ntf_file)= @_;
  my $topo= topo_from_ntf($ntf_file);
  $cbgp->send_cmd("net ntf load \"$ntf_file\"");
  return cbgp_topo_check_links($cbgp, $topo);
}
