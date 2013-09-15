return ["abilene", "cbgp_valid_abilene"];

# -----[ cbgp_valid_abilene ]----------------------------------------
#
# -------------------------------------------------------------------
sub cbgp_valid_abilene($) {
  my ($cbgp)= @_;

  my %nodes= (
	      "198.32.12.9" => "ATLA",
	      "198.32.12.25" => "CHIN",
	      "198.32.12.41" => "DNVR",
	      "198.32.12.57" => "HSTN",
	      "198.32.12.177" => "IPLS",
	      "198.32.12.89" => "KSCY",
	      "198.32.12.105" => "LOSA",
	      "198.32.12.121" => "NYCM",
	      "198.32.12.137" => "SNVA",
	      "198.32.12.153" => "STTL",
	      "198.32.12.169" => "WASH",
	     );

  $cbgp->send_cmd("net add domain 11537 igp");
  foreach my $r (keys %nodes) {
    my $name= $nodes{$r};
    $cbgp->send_cmd("net add node $r");
    $cbgp->send_cmd("net node $r domain 11537");
    $cbgp->send_cmd("net node $r name \"$name\"");
  }

  # Internal links
  $cbgp->send_cmd("net add link 198.32.12.137 198.32.12.41");
  $cbgp->send_cmd("net add link 198.32.12.137 198.32.12.105");
  $cbgp->send_cmd("net add link 198.32.12.137 198.32.12.153");
  $cbgp->send_cmd("net add link 198.32.12.41 198.32.12.153");
  $cbgp->send_cmd("net add link 198.32.12.153 198.32.12.89");
  $cbgp->send_cmd("net add link 198.32.12.105 198.32.12.57");
  $cbgp->send_cmd("net add link 198.32.12.89 198.32.12.57");
  $cbgp->send_cmd("net add link 198.32.12.89 198.32.12.177");
  $cbgp->send_cmd("net add link 198.32.12.57 198.32.12.9");
  $cbgp->send_cmd("net add link 198.32.12.9 198.32.12.177");
  $cbgp->send_cmd("net add link 198.32.12.9 198.32.12.169");
  $cbgp->send_cmd("net add link 198.32.12.25 198.32.12.177");
  $cbgp->send_cmd("net add link 198.32.12.25 198.32.12.121");
  $cbgp->send_cmd("net add link 198.32.12.169 198.32.12.121");

  # Compute IGP routes
  $cbgp->send_cmd("net link 198.32.12.137 198.32.12.41 igp-weight --bidir 1295");
  $cbgp->send_cmd("net link 198.32.12.137 198.32.12.105 igp-weight --bidir 366");
  $cbgp->send_cmd("net link 198.32.12.137 198.32.12.153 igp-weight --bidir 861");
  $cbgp->send_cmd("net link 198.32.12.41 198.32.12.153 igp-weight --bidir 2095");
  $cbgp->send_cmd("net link 198.32.12.153 198.32.12.89 igp-weight --bidir 639");
  $cbgp->send_cmd("net link 198.32.12.105 198.32.12.57 igp-weight --bidir 1893");
  $cbgp->send_cmd("net link 198.32.12.89 198.32.12.57 igp-weight --bidir 902");
  $cbgp->send_cmd("net link 198.32.12.89 198.32.12.177 igp-weight --bidir 548");
  $cbgp->send_cmd("net link 198.32.12.57 198.32.12.9 igp-weight --bidir 1176");
  $cbgp->send_cmd("net link 198.32.12.9 198.32.12.177 igp-weight --bidir 587");
  $cbgp->send_cmd("net link 198.32.12.9 198.32.12.169 igp-weight --bidir 846");
  $cbgp->send_cmd("net link 198.32.12.25 198.32.12.177 igp-weight --bidir 260");
  $cbgp->send_cmd("net link 198.32.12.25 198.32.12.121 igp-weight --bidir 700");
  $cbgp->send_cmd("net link 198.32.12.169 198.32.12.121 igp-weight --bidir 233");
  $cbgp->send_cmd("net domain 11537 compute");

  $cbgp->send_cmd("bgp add router 11537 198.32.12.9");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.25");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.41");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.57");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.177");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.89");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.105");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.121");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.137");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.153");
  $cbgp->send_cmd("bgp add router 11537 198.32.12.169");

  $cbgp->send_cmd("bgp domain 11537 full-mesh");

  $cbgp->send_cmd("sim run");

  my %ribs= (
	     "198.32.12.9" => "ATLA/rib.20050801.0124-ebgp.ascii",
	     "198.32.12.25" => "CHIN/rib.20050801.0017-ebgp.ascii",
	     "198.32.12.41" => "DNVR/rib.20050801.0014-ebgp.ascii",
	     "198.32.12.57" => "HSTN/rib.20050801.0149-ebgp.ascii",
	     "198.32.12.177" => "IPLS/rib.20050801.0157-ebgp.ascii",
	     "198.32.12.89" => "KSCY/rib.20050801.0154-ebgp.ascii",
	     "198.32.12.105" => "LOSA/rib.20050801.0034-ebgp.ascii",
	     "198.32.12.121" => "NYCM/rib.20050801.0050-ebgp.ascii",
	     "198.32.12.137" => "SNVA/rib.20050801.0118-ebgp.ascii",
	     "198.32.12.153" => "STTL/rib.20050801.0124-ebgp.ascii",
	     "198.32.12.169" => "WASH/rib.20050801.0101-ebgp.ascii",
	    );

  foreach my $r (keys %ribs) {
    my $rib= get_resource("abilene/".$ribs{$r});
    (-e $rib) or return TEST_DISABLED;
  }

  foreach my $r (keys %ribs) {
    my $rib= get_resource("abilene/".$ribs{$r});
    $cbgp->send_cmd("bgp router $r load rib --autoconf \"$rib\"");
  }
  $cbgp->send_cmd("sim run");

  foreach my $r (keys %ribs) {
    cbgp_get_rib($cbgp, $r);
  }

  return TEST_SUCCESS;
}

