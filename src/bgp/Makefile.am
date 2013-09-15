# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
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

SUBDIRS = aslevel attr filter
DIST_SUBDIRS = aslevel attr filter

AM_CFLAGS = $(OPT_EXPERIMENTAL) $(OPT_EXTERNAL_BEST) $(OPT_WALTON) -I..

noinst_LTLIBRARIES = libbgp.la
libbgp_la_CFLAGS = $(LIBGDS_CFLAGS)
libbgp_la_LIBADD = \
	aslevel/libbgp_aslevel.la \
	attr/libbgp_attr.la \
	filter/libbgp_filter.la

libbgp_la_SOURCES = \
	as.c \
	as.h \
	attr.c \
	attr.h \
	auto-config.c \
	auto-config.h \
	bgp_assert.h \
	bgp_assert.c \
	bgp_debug.h \
	bgp_debug.c \
	cisco.c \
	cisco.h \
	dp_rt.c \
	dp_rt.h \
	dp_rules.c \
	dp_rules.h \
	domain.c \
	domain.h \
	message.c \
	message.h \
	mrtd.c \
	mrtd.h \
	nlri.h \
	peer.c \
	peer.h \
	peer-list.c \
	peer-list.h \
	qos.c \
	qos.h \
	record-route.c \
	record-route.h \
	rib.c \
	rib.h \
	route.c \
	route.h \
	route_reflector.c \
	route_reflector.h \
	route_map.c \
	route_map.h \
	routes_list.c \
	routes_list.h \
	route-input.c \
	route-input.h \
	tie_breaks.c \
	tie_breaks.h \
	types.h \
	walton.c \
	walton.h
