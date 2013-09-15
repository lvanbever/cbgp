// ==================================================================
// @(#)export_cli.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 15/10/07
// $Id: export_cli.c,v 1.6 2009-03-24 16:07:49 bqu Exp $
//
// C-BGP, BGP Routing Solver
// Copyright (C) 2002-2008 Bruno Quoitin
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
// 02111-1307  USA
// ==================================================================
// Note:
//   - tunnels and tunnel-based routes are not (yet) suppported
//       (there might be an inter-dependency between routing and
//        forwarding in this case)
//   - BGP filters are not (yet) supported
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <time.h>

#include <libgds/enumerator.h>

#include <net/error.h>
#include <net/export_cli.h>
#include <net/link-list.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>
#include <bgp/filter/filter.h>
#include <bgp/peer.h>
#include <bgp/peer-list.h>
#include <bgp/route_map.h>
#include <bgp/types.h>

#define COMM_SEP1 \
  "# ===================================================================\n"
#define COMM_SEP2 \
  "# -------------------------------------------------------------------\n"

// -----[ _net_export_cli_header ]-----------------------------------
static void _net_export_cli_header(gds_stream_t * stream)
{
  time_t cur_time= time(NULL);

  stream_printf(stream, COMM_SEP1);
  stream_printf(stream, "# C-BGP Export file (CLI)\n");
  stream_printf(stream, "# generated on %s", ctime(&cur_time));
  stream_printf(stream, "# with version %s\n", PACKAGE_VERSION);
  stream_printf(stream, COMM_SEP1);
}

// -----[ _net_export_cli_comment ]----------------------------------
static void _net_export_cli_comment(gds_stream_t * stream,
				    const char * comment)
{
  stream_printf(stream, COMM_SEP2);
  stream_printf(stream, "# %s\n", comment);
  stream_printf(stream, COMM_SEP2);
}

// -----[ _export_net_node ]-----------------------------------------
static inline void _export_net_node(gds_stream_t * stream,
				    net_node_t * node)
{
  stream_printf(stream, "net add node --no-loopback ");
  node_dump(stream, node);
  stream_printf(stream, "\n");
}

// -----[ _export_net_subnet ]---------------------------------------
static inline void _export_net_subnet(gds_stream_t * stream,
				      net_subnet_t * subnet)
{
  stream_printf(stream, "net add subnet ");
  ip_prefix_dump(stream, subnet->prefix);
  stream_printf(stream, " ");
  switch (subnet->type) {
  case NET_SUBNET_TYPE_TRANSIT: stream_printf(stream, "transit"); break;
  case NET_SUBNET_TYPE_STUB: stream_printf(stream, "stub"); break;
  default:
    abort();
  }
  stream_printf(stream, "\n");
}

// -----[ _export_net_iface_lo ]-------------------------------------
static inline void _export_net_iface_lo(gds_stream_t * stream,
					net_iface_t * iface)
{
  stream_printf(stream, "net node ");
  node_dump_id(stream, iface->owner);
  stream_printf(stream, " add iface ");
  net_iface_dump_id(stream, iface);
  stream_printf(stream, " loopback\n");
}

// -----[ _export_net_link_rtr ]-------------------------------------
static inline void _export_net_link_rtr(gds_stream_t * stream,
					net_iface_t * iface)
{
  if (net_iface_is_connected(iface) &&
      (iface->owner < iface->dest.iface->owner))
    return;
  stream_printf(stream, "net add link ");
  if (iface->phys.delay != 0)
    stream_printf(stream, "--delay=%u ", iface->phys.delay);
  node_dump_id(stream, iface->owner);
  stream_printf(stream, " ");
  node_dump_id(stream, iface->dest.iface->owner);
  stream_printf(stream, "\n");
}

// -----[ _export_net_link_ptp ]-------------------------------------
static inline void _export_net_link_ptp(gds_stream_t * stream,
					net_iface_t * iface)
{
  if (net_iface_is_connected(iface) &&
      (iface->owner < iface->dest.iface->owner))
    return;
  stream_printf(stream, "net add link-ptp ");
  if (iface->phys.delay != 0)
    stream_printf(stream, "--delay=%u ", iface->phys.delay);
  node_dump_id(stream, iface->owner);
  stream_printf(stream, " ");
  net_iface_dump_id(stream, iface);
  stream_printf(stream, " ");
  node_dump_id(stream, iface->dest.iface->owner);
  stream_printf(stream, " ");
  net_iface_dump_id(stream, iface->dest.iface);
  stream_printf(stream, "\n");
}

// -----[ _export_net_link_ptmp ]------------------------------------
static inline void _export_net_link_ptmp(gds_stream_t * stream,
					 net_iface_t * iface)
{
  stream_printf(stream, "net add link ");
  if (iface->phys.delay != 0)
    stream_printf(stream, "--delay=%u ", iface->phys.delay);
  node_dump_id(stream, iface->owner);
  stream_printf(stream, " ");
  ip_address_dump(stream, iface->addr);
  stream_printf(stream, "/%u\n", iface->mask);
}

// -----[ _export_net_link_virtual ]---------------------------------
static inline void _export_net_link_virtual(gds_stream_t * stream,
					    net_iface_t * iface)
{
  stream_printf(stream, "#tunnel from ");
  node_dump_id(stream, iface->owner);
  stream_printf(stream, " ");
  net_iface_dump_id(stream, iface);
  stream_printf(stream, " (not supported by export)\n");
}

// -----[ _export_net_node_route ]-----------------------------------
static inline void _export_net_node_route(gds_stream_t * stream,
					  net_node_t * node,
					  rt_info_t * rtinfo)
{
  unsigned int index;
  rt_entry_t * rtentry;

  for (index= 0; index < rt_entries_size(rtinfo->entries); index++) {
    rtentry= rt_entries_get_at(rtinfo->entries, index);

    // Do not support static routes through tunnels
    if ((rtentry->oif != NULL) &&
	(rtentry->oif->type == NET_IFACE_VIRTUAL)) {
      stream_printf(stream, "# static routes from ");
      node_dump_id(stream, node);
      stream_printf(stream, " to ");
      ip_prefix_dump(stream, rtinfo->prefix);
      stream_printf(stream, " (not supported by export)\n");
      return;
    }

    stream_printf(stream, "net node ");
    node_dump_id(stream, node);
    stream_printf(stream, " route add");

    // Optional gateway ?
    if (rtentry->gateway != NET_ADDR_ANY) {
      stream_printf(stream, " --gw=");
      ip_address_dump(stream, rtentry->gateway);
    }

    // Optional outgoing interface (oif)
    if (rtentry->oif != NULL) {
      stream_printf(stream, " --oif=");
      net_iface_dump_id(stream, rtentry->oif);
    }
    
    // Destination prefix
    stream_printf(stream, " ");
    ip_prefix_dump(stream, rtinfo->prefix);
    
    // Metric
    stream_printf(stream, " %d\n", rtinfo->metric);
  }
}

// -----[ _net_export_cli_phys ]-------------------------------------
/**
 * Save all nodes, links, subnets
 */
static void _net_export_cli_phys(gds_stream_t * stream,
				 network_t * network)
{
  gds_enum_t * nodes, * links, * subnets;
  net_node_t * node;
  net_subnet_t * subnet;
  net_iface_t * link;

  // *** all nodes ***
  nodes= trie_get_enum(network->nodes);
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    _export_net_node(stream, node);
  }
  enum_destroy(&nodes);

  // *** all subnets ***
  subnets= _array_get_enum((array_t*) network->subnets);
  while (enum_has_next(subnets)) {
    subnet= *((net_subnet_t **) enum_get_next(subnets));
    _export_net_subnet(stream, subnet);
  }
  enum_destroy(&subnets);

  // *** all links ***
  nodes= trie_get_enum(network->nodes);
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));

    links= net_links_get_enum(node->ifaces);
    while (enum_has_next(links)) {
      link= *((net_iface_t **) enum_get_next(links));

      switch (link->type) {
      case NET_IFACE_LOOPBACK:
	_export_net_iface_lo(stream, link);
	break;
      case NET_IFACE_RTR:
	_export_net_link_rtr(stream, link);
	break;
      case NET_IFACE_PTP:
	_export_net_link_ptp(stream, link);
	break;
      case NET_IFACE_PTMP:
	_export_net_link_ptmp(stream, link);
	break;
      case NET_IFACE_VIRTUAL:
	_export_net_link_virtual(stream, link);
	break;
      default:
	abort();
      }

    }
    enum_destroy(&links);

    links= net_links_get_enum(node->ifaces);
    while (enum_has_next(links)) {
      link= *((net_iface_t **) enum_get_next(links));

      if (!net_iface_is_enabled(link)) {
	stream_printf(stream, "net node ");
	node_dump_id(stream, node);
	stream_printf(stream, " iface ");
	net_iface_dump_id(stream, link);
	stream_printf(stream, " down\n");
      }
    }
    enum_destroy(&links);

  }
  enum_destroy(&nodes);
}

// -----[ _net_export_cli_static ]-----------------------------------
/**
 * Static routes
 */
static void _net_export_cli_static(gds_stream_t * stream,
				   network_t * network)
{
  gds_enum_t * nodes, * rt_info_lists, * rt_infos;
  net_node_t * node;
  rt_info_list_t * rt_info_list;
  rt_info_t * rt_info;

  nodes= trie_get_enum(network->nodes);
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));

    rt_info_lists= trie_get_enum(node->rt);
    while (enum_has_next(rt_info_lists)) {
      rt_info_list= *((rt_info_list_t **) enum_get_next(rt_info_lists));

      /*for (index= 0; index < _rt_info_list_length(list); index++) {
    rt_info_dump(stream, (rt_info_t *) list->data[index]);
    stream_printf(stream, "\n");
    }*/


      rt_infos= _array_get_enum((array_t *) rt_info_list);
      while (enum_has_next(rt_infos)) {
	rt_info= *((rt_info_t **) enum_get_next(rt_infos));

	// Only static routes are exported
	if (rt_info->type != NET_ROUTE_STATIC)
	  continue;

	_export_net_node_route(stream, node, rt_info);
      }
      enum_destroy(&rt_infos);

    }
    enum_destroy(&rt_info_lists);
    
  }
  enum_destroy(&nodes);
}

static int _igp_domain_fe(igp_domain_t * domain, void * ctx)
{
  gds_enum_t * routers, * links;
  gds_stream_t * stream= (gds_stream_t *) ctx;
  net_node_t * router;
  net_iface_t * link;

  stream_printf(stream, "net add domain %d igp\n", domain->id);

  routers= trie_get_enum(domain->routers);
  while (enum_has_next(routers)) {
    router= *((net_node_t **) enum_get_next(routers));
    stream_printf(stream, "net node ");
    node_dump_id(stream, router);
    stream_printf(stream, " domain %d\n", domain->id);
  }
  enum_destroy(&routers);

  routers= trie_get_enum(domain->routers);
  while (enum_has_next(routers)) {
    router= *((net_node_t **) enum_get_next(routers));
    
    links= net_links_get_enum(router->ifaces);
    while (enum_has_next(links)) {
      link= *((net_iface_t **) enum_get_next(links));
      
      if ((link->type == NET_IFACE_VIRTUAL) ||
	  (link->type == NET_IFACE_LOOPBACK))
	continue;
      
      stream_printf(stream, "net node ");
      node_dump_id(stream, router);
      stream_printf(stream, " iface ");
      net_iface_dump_id(stream, link);
      stream_printf(stream, " igp-weight %u\n", net_iface_get_metric(link, 0));
    }
    enum_destroy(&links);

  }
  enum_destroy(&routers);
  
  stream_printf(stream, "net domain %d compute\n", domain->id);

  return 0;
}

// -----[ _net_export_cli_igp ]--------------------------------------
/**
 * IGP configuration (domains, weights)
 */
static void _net_export_cli_igp(gds_stream_t * stream,
				network_t * network)
{
  igp_domains_for_each(network->domains, _igp_domain_fe, stream);
}

// -----[ _export_bgp_router_network ]-------------------------------
static inline void _export_bgp_router_networks(gds_stream_t * stream,
					       bgp_router_t * router)
{
}

// -----[ _export_bgp_filter ]---------------------------------------
static void _export_bgp_filter(gds_stream_t * stream,
			       bgp_filter_t * filter,
			       const char * pfx_str)
{
  unsigned int index;
  bgp_ft_rule_t * rule;
  bgp_ft_action_t * action;

  for (index= 0; index < filter->rules->size; index++) {
    rule= (bgp_ft_rule_t *) filter->rules->items[index];
    stream_printf(stream, "%sadd-rule\n", pfx_str);
    stream_printf(stream, "%s  match '", pfx_str);
    filter_matcher_dump(stream, rule->matcher);
    stream_printf(stream, "'\n");
    action= rule->action;
    while (action != NULL) {
      stream_printf(stream, "%s  action \"", pfx_str);
      filter_action_dump(stream, action);
      action= action->next_action;
      stream_printf(stream, "\"\n");
    }
    stream_printf(stream, "%s  exit\n", pfx_str);
  }
  stream_printf(stream, "%sexit\n", pfx_str);
}

// -----[ _export_bgp_router_peer ]----------------------------------
static inline void _export_bgp_router_peer(gds_stream_t * stream,
					   bgp_peer_t * peer)
{
  stream_printf(stream, "  add peer %u ", peer->asn);
  ip_address_dump(stream, peer->addr);
  stream_printf(stream, "\n");
  stream_printf(stream, "  peer ");
  ip_address_dump(stream, peer->addr);
  stream_printf(stream, "\n");
  if (peer->flags & PEER_FLAG_RR_CLIENT)
    stream_printf(stream, "    rr-client\n");
  if (peer->flags & PEER_FLAG_NEXT_HOP_SELF)
    stream_printf(stream, "    next-hop-self\n");
  if (peer->flags & PEER_FLAG_SOFT_RESTART)
    stream_printf(stream, "    soft-restart\n");
  if (peer->flags & PEER_FLAG_VIRTUAL)
    stream_printf(stream, "    virtual\n");
  if (peer->next_hop != NET_ADDR_ANY) {
    stream_printf(stream, "    next-hop ");
    ip_address_dump(stream, peer->next_hop);
    stream_printf(stream, "\n");
  }
  if (peer->src_addr != NET_ADDR_ANY) {
    stream_printf(stream, "    update-source ");
    ip_address_dump(stream, peer->src_addr);
    stream_printf(stream, "\n");
  }
  if (peer->session_state != SESSION_STATE_IDLE)
    stream_printf(stream, "    up\n");
  if (peer->filter[FILTER_IN] != NULL) {
    stream_printf(stream, "    filter in\n");
    _export_bgp_filter(stream, peer->filter[FILTER_IN], "      ");
    stream_printf(stream, "    exit\n");
  }
  if (peer->filter[FILTER_OUT] != NULL) {
    stream_printf(stream, "    filter out\n");
    _export_bgp_filter(stream, peer->filter[FILTER_OUT], "      ");
    stream_printf(stream, "    exit\n");
  }
  stream_printf(stream, "  exit\n");
}

// -----[ _export_bgp_router ]---------------------------------------
static inline void _export_bgp_router(gds_stream_t * stream,
				      bgp_router_t * router)
{
  unsigned int index;

  stream_printf(stream, "bgp add router %u ", router->asn);
  ip_address_dump(stream, router->node->rid);
  stream_printf(stream, "\n");
  stream_printf(stream, "bgp router ");
  ip_address_dump(stream, router->node->rid);
  stream_printf(stream, "\n");
  for (index= 0; index < bgp_peers_size(router->peers); index++)
    _export_bgp_router_peer(stream, router->peers->data[index]);
  stream_printf(stream, "\n");
  _export_bgp_router_networks(stream, router);
}

// -----[ _net_export_cli_bgp_route_maps ]---------------------------
static void _net_export_cli_bgp_route_maps(gds_stream_t * stream)
{
  gds_enum_t * enu= route_map_enum();
  _route_map_t * rm;
  while (enum_has_next(enu)) {
    rm= *((_route_map_t **) enum_get_next(enu));
    stream_printf(stream, "bgp route-map \"%s\"\n", rm->name);
    _export_bgp_filter(stream, rm->filter, "  ");
    stream_printf(stream, "\n");
  }
}

// -----[ _net_export_cli_bgp ]--------------------------------------
/**
 * BGP configuration
 */
static void _net_export_cli_bgp(gds_stream_t * stream,
				network_t * network)
{
  gds_enum_t * nodes= trie_get_enum(network->nodes);
  net_node_t * node;
  net_protocol_t * proto;

  _net_export_cli_bgp_route_maps(stream);

  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    proto= node_get_protocol(node, NET_PROTOCOL_BGP);
    if (proto == NULL)
      continue;
    _export_bgp_router(stream, (bgp_router_t *) proto->handler);
  }
  enum_destroy(&nodes);
}

// -----[ net_export_cli ]-------------------------------------------
net_error_t net_export_cli(gds_stream_t * stream, network_t * network)
{
  _net_export_cli_header(stream);
  stream_printf(stream, "\n");

  _net_export_cli_comment(stream, "Physical topology");
  _net_export_cli_phys(stream, network);
  stream_printf(stream, "\n");

  _net_export_cli_comment(stream, "Static routing");
  _net_export_cli_static(stream, network);
  stream_printf(stream, "\n");

  _net_export_cli_comment(stream, "IGP routing"); 
  _net_export_cli_igp(stream, network);
  stream_printf(stream, "\n");

  _net_export_cli_comment(stream, "BGP routing"); 
  _net_export_cli_bgp(stream, network);

  return ESUCCESS;
}
