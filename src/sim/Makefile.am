# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL) -I..

noinst_LTLIBRARIES = libsim.la
libsim_la_CFLAGS = $(LIBGDS_CFLAGS)

libsim_la_SOURCES = \
	scheduler.c \
	scheduler.h \
	simulator.c \
	simulator.h \
	static_scheduler.c \
	static_scheduler.h
