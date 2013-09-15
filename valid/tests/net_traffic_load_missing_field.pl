use strict;

return ["net traffic load (missing field)",
	"cbgp_valid_net_traffic_load_missing_field"];

# -----[ cbgp_valid_net_traffic_load_missing_field ]-----------------
# Check that an error is returned if one tries to load a traffic
# matrix with a missing field.
# -------------------------------------------------------------------
sub cbgp_valid_net_traffic_load_missing_field($) {
  my ($cbgp)= @_;
  my $info;

  my $filename= get_resource("valid-net-tm-missing-field.tm");
  (-e $filename) or return TEST_MISSING;

  my $msg= cbgp_check_error($cbgp, "net traffic load \"$filename\"");
  return TEST_FAILURE
    if (!check_has_error($msg, "required field missing"));

  return TEST_SUCCESS;
}

