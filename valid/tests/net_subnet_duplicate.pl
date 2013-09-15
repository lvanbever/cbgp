use strict;

return ["net subnet duplicate",
	"cbgp_valid_net_subnet_duplicate"];

# -----[ cbgp_valid_net_subnet_duplicate ]---------------------------
# Check that it is not possible to add the same subnet twice.
# -------------------------------------------------------------------
sub cbgp_valid_net_subnet_duplicate($) {
  my ($cbgp)= @_;
  my $msg;

  $cbgp->send_cmd("net add subnet 192.168.0/24 transit\n");
  $msg= cbgp_check_error($cbgp, "net add subnet 192.168.0/24 transit\n");
  return TEST_FAILURE
    if (!check_has_error($msg, "subnet already exists"));
  $msg= cbgp_check_error($cbgp, "net add subnet 192.168.0.1/24 transit\n");
  return TEST_FAILURE
    if (!check_has_error($msg, "subnet already exists"));
  return TEST_SUCCESS;
}

