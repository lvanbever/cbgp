# Compile experimental features ?
if WITH_EXPERIMENTAL
  OPT_EXPERIMENTAL = -D__EXPERIMENTAL__
else
  OPT_EXPERIMENTAL =
endif

AM_CFLAGS = $(OPT_EXPERIMENTAL) $(OPT_DEBUGGING) -I..

noinst_LTLIBRARIES = libjni.la

SUBDIRS = impl
DIST_SUBDIRS = $(SUBDIRS)

libjni_la_SOURCES = \
	jni_base.c \
	jni_base.h \
	jni_interface.c \
	jni_proxies.c \
	jni_proxies.h \
	jni_util.c \
	jni_util.h \
	exceptions.c \
	exceptions.h \
	listener.c \
	listener.h
libjni_la_LIBADD = impl/libjni_impl.la
libjni_la_CFLAGS = $(LIBGDS_CFLAGS) -Wall -Werror -O2 -g

JARFILE = CBGP.jar
BUILT_SOURCES = $(JARFILE)

PKG_DIR=be/ac/ucl/ingi/cbgp

JAVA_PACKAGES = \
	be.ac.ucl.ingi.cbgp \
	be.ac.ucl.ingi.cbgp.bgp \
	be.ac.ucl.ingi.cbgp.exceptions \
	be.ac.ucl.ingi.cbgp.net

JAVAFILES= \
	BGPMsgListener.java \
	CBGP.java \
	exceptions/CBGPException.java \
	exceptions/CBGPScriptException.java \
	exceptions/InvalidDestinationException.java \
	ConsoleEvent.java \
	ConsoleEventListener.java \
	AbstractConsoleEventListener.java \
	IPAddress.java \
	IPPrefix.java \
	IPRoute.java \
	IPTrace.java \
	IPTraceElement.java \
	IPTraceNode.java \
	IPTraceSubnet.java \
	IPTraceTrace.java \
	LinkMetrics.java \
	exceptions/UnknownMetricException.java \
	ProxyObject.java \
	Route.java \
	RouteEntry.java \
	RouteInterface.java \
	bgp/ASPath.java \
	bgp/ASPathSegment.java \
	bgp/Communities.java \
	bgp/Domain.java \
	bgp/Route.java \
	bgp/Filter.java \
	bgp/FilterRule.java \
	bgp/MessageUpdate.java \
	bgp/MessageWithdraw.java \
	bgp/Peer.java \
	bgp/Router.java \
	net/Element.java \
	net/IGPDomain.java \
	net/Interface.java \
	net/Message.java \
	net/Node.java \
	net/Subnet.java

JAVATESTFILES= \
	testing/TestBGPDomain.java \
	testing/TestBGPRouter.java \
	testing/TestIGPDomain.java \
	testing/TestLink.java \
	testing/TestMisc.java \
	testing/TestNode.java \
	testing/TestSession.java \
	testing/TestSubnet.java \
	testing/TestScript.java \
	testing/TestBGPRoutes.java \
	testing/TestRecordRoute.java \
	testing/TestTraceRoute.java \
	testing/AllTests.java

# Java classes that need to be processed by javah to produce a
# C header file must be listed below
JNICLASSFILES= \
	be.ac.ucl.ingi.cbgp.CBGP \
	be.ac.ucl.ingi.cbgp.IPTrace \
	be.ac.ucl.ingi.cbgp.ProxyObject \
	be.ac.ucl.ingi.cbgp.bgp.Domain \
	be.ac.ucl.ingi.cbgp.bgp.Filter \
	be.ac.ucl.ingi.cbgp.bgp.FilterRule \
	be.ac.ucl.ingi.cbgp.bgp.Peer \
	be.ac.ucl.ingi.cbgp.bgp.Route \
	be.ac.ucl.ingi.cbgp.bgp.Router \
	be.ac.ucl.ingi.cbgp.net.IGPDomain \
	be.ac.ucl.ingi.cbgp.net.Interface \
	be.ac.ucl.ingi.cbgp.net.Node \
	be.ac.ucl.ingi.cbgp.net.Subnet

METAFILE= META-INF/MANIFEST.MF

ALLJAVAFILES=$(patsubst %,$(PKG_DIR)/%, \
	$(JAVAFILES) \
	$(JAVATESTFILES) \
	)

CLASSFILES=$(ALLJAVAFILES:%.java=%.class)

EXTRA_DIST = \
	$(ALLJAVAFILES) \
	$(METAFILE) \
	jni-test-wrapper.sh

CP = .:$(top_builddir)/lib/junit-4.4.jar
if HOST_IS_CYGWIN
  CLASSPATH = `cygpath -wp $(CP)`
else
  CLASSPATH = $(CP)
endif

# Invoke javac in order to build Java classes. In addition, invoke
# javah in order to build the native function headers.
.java.class:
	$(JAVAC) -classpath $(CLASSPATH) $<

# Old way to build JNI headers was through the generic .class -> .java
# rule. This was not working since 'javac' sometimes not only builds
# the specified target but also dependencies.
#(CLASSNAME=`echo $@ | sed -e 's/\//./g' -e 's/.class//g'`; \
#  $(JAVAH) -d headers $$CLASSNAME)

SUFFIXES= .java .class

# Build the jar file
$(JARFILE): $(CLASSFILES)
	@(for f in $(JNICLASSFILES); do \
	 echo "$(JAVAH) -d headers $$f"; \
	 $(JAVAH) -classpath $(CLASSPATH) -d headers $$f; \
	done)
	(jar cvfm $@ $(METAFILE) $(CLASSFILES))

CLEANFILES = $(JARFILE) $(CLASSFILES)


# -------------------------------------------------------------------
# Java tests
# -------------------------------------------------------------------

#TESTS = jni-test-wrapper.sh


# -------------------------------------------------------------------
# Java documentation
# -------------------------------------------------------------------

doc:
	$(JAVADOC) -d javadoc -classpath $(CLASSPATH) $(JAVA_PACKAGES)

plop:
	@echo "$(ALLJAVAFILES2)"

