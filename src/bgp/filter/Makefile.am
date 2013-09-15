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

noinst_LTLIBRARIES = libbgp_filter.la
libbgp_filter_la_CFLAGS = $(LIBGDS_CFLAGS)

libbgp_filter_la_SOURCES = \
	filter.c \
	filter.h \
	parser.c \
	parser.h \
	predicate_parser.c \
	predicate_parser.h \
	registry.c \
	registry.h \
	types.h
