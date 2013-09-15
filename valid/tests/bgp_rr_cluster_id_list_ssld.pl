return ["bgp RR cluster-id-list ssld",
	"cbgp_valid_bgp_rr_clssld"];

# -----[ cbgp_valid_bgp_rr_clssld ]-----------------------------------
# Test route-reflector ability to avoid cluster-id-list loop creation
# from the sender side. In this case, SSLD means "Sender-Side
# cluster-id-list Loop Detection".
#
# Setup:
#
# Scenario:
#
# -------------------------------------------------------------------
sub cbgp_valid_bgp_rr_clssld($) {
  return TEST_TODO;
}


