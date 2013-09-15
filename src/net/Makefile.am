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

AM_CFLAGS = $(OPT_EXPERIMENTAL) $(OPT_WITH_OSPF) -I..

SUBDIRS = traffic
DIST_SUBDIRS = traffic

noinst_LTLIBRARIES = libnet.la
libnet_la_CFLAGS= $(LIBGDS_CFLAGS)
libnet_la_LIBADD = \
	traffic/libnet_traffic.la

libnet_la_SOURCES = \
	error.c \
	error.h \
	export.c \
	export.h \
	export_cli.c \
	export_cli.h \
	export_graphviz.c \
	export_graphviz.h \
	export_ntf.c \
	export_ntf.h \
	ez_topo.c \
	ez_topo.h \
	icmp.c \
	icmp.h \
	icmp_options.c \
	icmp_options.h \
	iface.c \
	iface.h \
	iface_ptmp.c \
	iface_ptmp.h \
	iface_ptp.c \
	iface_ptp.h \
	iface_rtr.c \
	iface_rtr.h \
	igp.c \
	igp.h \
	igp_domain.c \
	igp_domain.h \
	ip.h \
	ip6.h \
	ipip.c \
	ipip.h \
	ip_trace.c \
	ip_trace.h \
	link.c \
	link.h \
	link_attr.c \
	link_attr.h \
	link-list.c \
	link-list.h \
	message.c \
	message.h \
	net_path.c \
	net_path.h \
	net_types.h \
	netflow.c \
	netflow.h \
	network.c \
	network.h \
	node.c \
	node.h \
	ntf.c \
	ntf.h \
	ospf.c \
	ospf.h \
	ospf_deflection.c \
	ospf_deflection.h \
	ospf_rt.c \
	ospf_rt.h \
	prefix.c \
	prefix.h \
	protocol.c \
	protocol.h \
	routing.c \
	routing.h \
	routing_t.h \
	rt_filter.c \
	rt_filter.h \
	spt.c \
	spt.h \
	spt_vertex.c \
	spt_vertex.h \
	subnet.c \
	subnet.h \
	tm.c \
	tm.h \
	util.c \
	util.h
	 

