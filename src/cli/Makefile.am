# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

if WITH_OSPF
  OPT_WITH_OSPF = -DOSPF_SUPPORT
else
  OPT_WITH_OSPF =
endif

if WITH_EXTERNAL_BEST
  OPT_EXTERNAL_BEST = -D__EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
else
  OPT_EXTERNAL_BEST = 
endif

if WITH_WALTON
  OPT_WALTON = -D__EXPERIMENTAL_WALTON__
else
  OPT_WALTON = 
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL) $(OPT_EXTERNAL_BEST) $(OPT_WALTON) $(OPT_WITH_OSPF) -I..

noinst_LTLIBRARIES = libcli.la
libcli_la_CFLAGS = $(LIBGDS_CFLAGS) $(READLINE_CFLAGS) $(AM_CFLAGS)

libcli_la_SOURCES = \
	bgp.c \
	bgp.h \
	bgp_filter.c \
	bgp_filter.h \
	bgp_router_peer.c \
	bgp_router_peer.h \
	bgp_topology.c \
	bgp_topology.h \
	common.c \
	common.h \
	context.h \
	enum.c \
	enum.h \
	net.c \
	net.h \
	net_domain.c \
	net_domain.h \
	net_node.c \
	net_node.h \
	net_node_iface.c \
	net_node_iface.h \
	net_ospf.c \
	net_ospf.h \
	sim.c \
	sim.h
