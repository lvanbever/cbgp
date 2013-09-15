return ["net ntf load (error)", "cbgp_valid_net_ntf_load_error"];

# -----[ cbgp_valid_net_ntf_load_error ]-----------------------------
# Load a topology from an NTF file into C-BGP (using C-BGP 's "net
# ntf load" command). Check that the links are correctly setup.
#
# Resources:
#   [valid-record-route.ntf]
# -------------------------------------------------------------------
sub cbgp_valid_net_ntf_load_error($) {
  my ($cbgp)= @_;
  my $filename= get_tmp_resource("net-ntf-load-error.ntf");

  die if !open(NTF, ">$filename");
  print NTF "1.0.0.1 1.0.0.0 2\n";
  print NTF "1.0.0.A 1.0.0.0 3\n";
  close(NTF);

  my $msg= cbgp_check_error($cbgp, "net ntf load \"$filename\"");
  (!check_has_error($msg, "invalid source .*, at line 2")) and
    return TEST_FAILURE;
  unlink $filename;
  return TEST_SUCCESS;
}

