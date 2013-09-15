return ["bgp topology load", "cbgp_valid_bgp_topology_load"];

# -----[ cbgp_valid_bgp_topology_load ]------------------------------
# Test ability to load a file in "Subramanian" format. See paper by
# Subramanian et al (Berkeley) at INFOCOM'02 about "Characterizing the
# Internet hierarchy".
#
# Setup:
#   - AS1 <--> AS2
#   - AS1 ---> AS3
#   - AS1 ---> AS4
#   - AS2 ---> AS5
#   - AS3 ---> AS6
#   - AS4 ---> AS6
#   - AS4 ---> AS7
#   - AS5 ---> AS7
#
# Scenario:
#   * One router per AS must have been created with an IP address
#     that corresponds to "ASx.ASy.0.0".
#   * Links must have been created where a business relationship
#     exists.
#
# Resources:
#   [valid-bgp-topology.subramanian]
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_load($) {
  my ($cbgp)= @_;

  my $topo_file= get_resource("valid-bgp-topology.subramanian");

  (-e $topo_file) or return TEST_SKIPPED;

  my $topo= topo_from_subramanian_file($topo_file);

  $cbgp->send_cmd("bgp topology load \"$topo_file\"");
  $cbgp->send_cmd("bgp topology install");

  # Check that all links are created
  (!cbgp_topo_check_links($cbgp, $topo)) and
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

