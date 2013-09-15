use strict;

return ["net traffic load (invalid field)",
	"cbgp_valid_net_traffic_load_invalid_field"];

# -----[ cbgp_valid_net_traffic_load_invalid_field ]-----------------
# Check that an error is returned if one tries to load a traffic
# matrix where the number of fields in a record does not match the 
# number of fields in the header.
# -------------------------------------------------------------------
sub cbgp_valid_net_traffic_load_invalid_field($) {
  my ($cbgp)= @_;
  my $info;

  my $filename= get_resource("valid-net-tm-invalid-field.tm");
  (-e $filename) or return TEST_MISSING;

  my $msg= cbgp_check_error($cbgp, "net traffic load \"$filename\"");
  return TEST_FAILURE
    if (!check_has_error($msg, "invalid header field"));

  return TEST_SUCCESS;
}

