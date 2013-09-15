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

noinst_LTLIBRARIES = libbgp_aslevel.la
libbgp_aslevel_la_CFLAGS = $(LIBGDS_CFLAGS)

libbgp_aslevel_la_SOURCES = \
	as-level.c \
	as-level.h \
	caida.c \
	caida.h \
	filter.c \
	filter.h \
	meulle.c \
	meulle.h \
	rexford.c \
	rexford.h \
	stat.c \
	stat.h \
	types.h \
	util.c \
	util.h
