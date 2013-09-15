# Compile with JNI ?
if WITH_JNI
  OPT_JNI_DIR = jni
  OPT_JNI_LIBADD = jni/libjni.la
else
  OPT_JNI_DIR =
  OPT_JNI_LIBADD =
endif

# Compile with ospf support?
if WITH_OSPF
  OPT_WITH_OSPF = -DOSPF_SUPPORT
else
  OPT_WITH_OSPF =
endif

# Compile with Electric Fence ?
if WITH_EFENCE
  OPT_EFENCE = -lefence
else
  OPT_EFENCE =
endif

# Compile with libgpdump ?
if WITH_BGPDUMP
  OPT_BGPDUMP_LIBADD = external/libexternal.la
  OPT_BGPDUMP_DIR = external
else
  OPT_BGPDUMP_LIBADD =
  OPT_BGPDUMP_DIR = 
endif

SUBDIRS = bgp cli net sim ui util $(OPT_JNI_DIR) $(OPT_BGPDUMP_DIR)
DIST_SUBDIRS = bgp cli external net sim ui util jni

# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL) $(OPT_WITH_OSPF)

lib_LTLIBRARIES = libcsim.la
libcsim_la_SOURCES = \
	api.c \
	selfcheck.c \
	selfcheck.h
libcsim_la_LIBADD = \
	bgp/libbgp.la \
	cli/libcli.la \
	sim/libsim.la \
	net/libnet.la \
	ui/libui.la \
	util/libutil.la \
	$(OPT_JNI_LIBADD) \
	$(OPT_LIBALLOC_LIBADD) \
	$(OPT_BGPDUMP_LIBADD) \
	$(OPT_EFENCE)
libcsim_la_LDFLAGS = -no-undefined -release $(CBGP_LT_RELEASE) \
	$(READLINE_LIBS) \
	$(LIBGDS_LIBS) \
	$(PCRE_LIBS)
libcsim_la_CFLAGS = \
	$(LIBGDS_CFLAGS) \
	$(AM_CFLAGS)
libcsiminclude_HEADERS = api.h
libcsimincludedir = $(includedir)/cbgp

if HOST_IS_CYGWIN
  bin_PROGRAMS = cbgp cbgp-test
else
  bin_PROGRAMS = cbgp cbgp-dump cbgp-test cbgp-perf
endif

cbgp_SOURCES = main.c
cbgp_LDADD = libcsim.la 
cbgp_CFLAGS = $(LIBGDS_CFLAGS) $(AM_CFLAGS)
cbgp_LDFLAGS = $(LIBGDS_LIBS)

cbgp_dump_SOURCES = main-dump.c
cbgp_dump_LDADD = libcsim.la
cbgp_dump_CFLAGS = $(LIBGDS_CFLAGS) $(AM_CFLAGS)
cbgp_dump_LDFLAGS = $(LIBGDS_LIBS)

cbgp_test_SOURCES = main-test.c
cbgp_test_LDADD = libcsim.la
cbgp_test_CFLAGS = $(LIBGDS_CFLAGS) $(AM_CFLAGS)
cbgp_test_LDFLAGS = $(LIBGDS_LIBS)

cbgp_perf_SOURCES = main-perf.c
cbgp_perf_LDADD = libcsim.la
cbgp_perf_CFLAGS = $(LIBGDS_CFLAGS) $(AM_CFLAGS)
cbgp_perf_LDFLAGS = $(LIBGDS_LIBS)

TESTS = \
	cbgp-test

# -------------------------------------------------------------------
# Install hook (called after all install targets/hooks)
# 1). Copy the CBPG.jar Java archive to the JNI install dir.
# 2). Under Mac OS X (Darwin), create a symlink from libcsim.jnilib to
# libcsim.dylib in $libdir . This is required for correct JNI
# operation.
# -------------------------------------------------------------------
if WITH_JNI
install-exec-local: 
	$(INSTALL) jni/CBGP.jar $(libdir)/CBGP.jar
	(case @host_os@ in \
	 darwin*) \
	 rm -f $(libdir)/libcsim.jnilib; \
	 cd $(libdir); \
	 @LN_S@ $(libdir)/libcsim.dylib libcsim.jnilib; \
	 cd -; \
	 ;; \
	 esac)
endif

# -------------------------------------------------------------------
# Documentation
# -------------------------------------------------------------------
.PHONY: doc

DOC_SUBDIRS = $(OPT_JNI_DIR)

doc:
	@(for subdir in $(DOC_SUBDIRS); do \
	(echo "$$subdir" ; cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) doc) \
	done)

dist-hook:
	(for f in `find $(distdir) | grep -E '\.(c|h|java)$$'`; do \
	echo $$f; \
	sed -E -e '/\$$Id.*\$$/ r $(top_builddir)/LICENCE' $$f > $$f.tmp; \
	mv $$f.tmp $$f; \
	done)
