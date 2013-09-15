use strict;

return ["bgp topology filter asn",
	"cbgp_valid_bgp_topology_filter_asn"];

# -----[ cbgp_valid_bgp_topology_filter_asn ]------------------------
# This test checks that it is possible to remove a single domain
# based on its ASN from an AS-level topology.
#
# Setup:
#   - AS1 provider of {AS3, AS4}
#   - AS2 provider of {AS3, AS4, AS7}
#   - AS3 provider of {AS5, AS6}
#   - AS4 provider of {AS6, AS7}
#   - AS5
#   - AS6
#   - AS7
#
# Topology:
#
#     1 --- 2
#     | \ / |\
#     |  \  | \
#     | / \ |  |
#     3 --- 4  |
#    /\    /\  |
#   /  \  /  \ |
#  /    \/    \|
#  5     6     7
#
# Scenario:
#   * Load the whole topology
#   * Check presence of AS1..AS7
#   * Filter domain with ASN=4
#   * Check presence of AS1..AS3 and AS5..AS7
#   * Check absence of AS4
# -------------------------------------------------------------------
sub cbgp_valid_bgp_topology_filter_asn($) {
  my ($cbgp)= @_;
  my $filename= get_resource("as-level-topo-small.txt");
  (-e $filename) or return TEST_DISABLED;

  my $options= "--addr-sch=local";

  $cbgp->send_cmd("bgp topology load $options \"$filename\"");
  cbgp_checkpoint($cbgp);
  my $error= cbgp_check_error($cbgp, "bgp topology check");
  return TEST_FAILURE
    if (defined($error));

  # Obtain topology from cbgp
  my $topo= cbgp_bgp_topology($cbgp, C_TOPO_ADDR_SCH_ASN);

  # Check that AS1-7 exist
  foreach my $asn (1, 2, 3, 4, 5, 6, 7) {
    return TEST_FAILURE
      if (!topo_check_asn($topo, $asn));
  }

  # Filter domain with ASN=4
  $cbgp->send_cmd("bgp topology filter asn 4");

  # Obtain topology from cbgp
  $topo= cbgp_bgp_topology($cbgp, C_TOPO_ADDR_SCH_ASN);

  # Check that AS1-3, AS5-7 exist
  foreach my $asn ((1, 2, 3, 5, 6, 7)) {
    return TEST_FAILURE
      if (!topo_check_asn($topo, $asn));
  }
  # Check that AS4 does not exist
  return TEST_FAILURE
    if (topo_check_asn($topo, 4));

  return TEST_SUCCESS;
}
