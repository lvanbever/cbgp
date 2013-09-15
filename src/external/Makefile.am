# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL)

noinst_LTLIBRARIES = libexternal.la

libexternal_la_SOURCES = \
	bgpdump-config.h \
	bgpdump.h \
	bgpdump_attr.h \
	bgpdump_formats.h \
	bgpdump_lib.h \
	bgpdump_mstream.h \
	bgpdump_lib.c \
	bgpdump_mstream.c \
	cfile_tools.h \
	cfile_tools.c
