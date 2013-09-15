return ["net link weight bidir", "cbgp_valid_net_link_weight_bidir"];

# -----[ cbgp_valid_net_link_weight_bidir ]--------------------------
# Check that the IGP weight of a link can be defined in a single
# direction or in both directions at a time.
# -------------------------------------------------------------------
sub cbgp_valid_net_link_weight_bidir($)
  {
    my ($cbgp)= @_;
    my $links;

    $cbgp->send_cmd("net add node 1.0.0.1");
    $cbgp->send_cmd("net add node 1.0.0.2");
    $cbgp->send_cmd("net add link 1.0.0.1 1.0.0.2");

    $links= cbgp_get_links($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_link($links, '1.0.0.2',
			  -weight=>MAX_IGP_WEIGHT));
    $links= cbgp_get_links($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.1",
			  -weight=>MAX_IGP_WEIGHT));

    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight 123");
    $links= cbgp_get_links($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.2",
			  -weight=>123));
    $links= cbgp_get_links($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.1",
			  -weight=>MAX_IGP_WEIGHT));

    $cbgp->send_cmd("net link 1.0.0.1 1.0.0.2 igp-weight --bidir 456");
    $links= cbgp_get_links($cbgp, "1.0.0.1");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.2",
			  -weight=>456));
    $links= cbgp_get_links($cbgp, "1.0.0.2");
    return TEST_FAILURE
      if (!check_has_link($links, "1.0.0.1",
			  -weight=>456));

    return TEST_SUCCESS;
  }

