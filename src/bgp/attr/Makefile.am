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

AM_CFLAGS = $(OPT_EXPERIMENTAL) $(OPT_EXTERNAL_BEST) $(OPT_WALTON) -I../..

noinst_LTLIBRARIES = libbgp_attr.la
libbgp_attr_la_CFLAGS = $(LIBGDS_CFLAGS)

libbgp_attr_la_SOURCES = \
	comm.h \
	comm.c \
	comm_hash.c \
	comm_hash.h \
	ecomm.h \
	ecomm.c \
	origin.c \
	origin.h \
	path.h \
	path.c \
	path_hash.c \
	path_hash.h \
	path_segment.h \
	path_segment.c \
	types.h
