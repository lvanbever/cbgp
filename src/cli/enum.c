// ==================================================================
// @(#)enum.c
//
// Enumeration functions used by the CLI.
//
// These functions are NOT thread-safe and MUST only be called from
// the CLI !!!
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be), 
//
// @date 27/04/2007
// $Id: enum.c,v 1.9 2009-08-31 09:37:41 bqu Exp $
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <cli/enum.h>
#include <bgp/as.h>
#include <bgp/peer.h>
#include <bgp/peer-list.h>
#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/types.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>

#define ASN_STR_LEN 6
#define IP4_ADDR_STR_LEN 16

static bgp_router_t * _ctx_bgp_router= NULL;
static as_level_domain_t * _ctx_as= NULL;

// -----[ cli_enum_ctx_bgp_router ]----------------------------------
void cli_enum_ctx_bgp_router(bgp_router_t * router)
{
  as_level_topo_t * topo;

  // Check if this router's ASN has a corresponding AS-level node
  if (router != _ctx_bgp_router) {
    if (router == NULL) {
      _ctx_as= NULL;
      return;
    }

    topo= aslevel_get_topo();
    if (topo == NULL) {
      _ctx_as= NULL;
      return;
    }

    _ctx_as= aslevel_topo_get_as(topo, router->asn);
  }

  _ctx_bgp_router= router;

}

// -----[ cli_enum_as_level_domains ]--------------------------------
as_level_domain_t * cli_enum_as_level_domains(const char * text,
					      int state)
{
  static as_level_topo_t * topo= NULL;
  static unsigned int index= 0;
  char str_asn[ASN_STR_LEN];
  as_level_domain_t * domain;

  if (state == 0) {
    topo= aslevel_get_topo();
    if (topo == NULL)
      return NULL;
    index= 0;
  }

  while (index < ptr_array_length(topo->domains)) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    assert(snprintf(str_asn, ASN_STR_LEN, "%u", domain->asn) >= 0);
    index++;
    if (!strncmp(text, str_asn, strlen(text)))
      return domain;
  }

  return NULL;
}

// -----[ cli_enum_net_nodes ]---------------------------------------
net_node_t * cli_enum_net_nodes(const char * text, int state)
{
  static gds_enum_t * nodes= NULL;
  net_node_t * node;
  char str_addr[IP4_ADDR_STR_LEN];

  if (state == 0)
    nodes= trie_get_enum(network_get_default()->nodes);

  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));

    // Optionally check if prefix matches
    if (text != NULL) {

      assert(ip_address_to_string(node->rid, str_addr,
				  sizeof(str_addr)) >= 0);
      if (strncmp(text, str_addr, strlen(text)))
	continue;
    }

    return node;
  }
  enum_destroy(&nodes);
  return NULL;
}

// -----[ cli_enum_net_nodes_id ]------------------------------------
net_node_t * cli_enum_net_nodes_id(const char * text, int state)
{
  as_level_domain_t * domain;
  if (strncmp("AS", text, 2))
    return cli_enum_net_nodes(text, state);
  domain= cli_enum_as_level_domains(text, state);
  if (domain == NULL)
    return NULL;
  if (domain->router == NULL)
    return NULL;
  return domain->router->node;
}

// -----[ cli_enum_bgp_routers ]-------------------------------------
bgp_router_t * cli_enum_bgp_routers(const char * text, int state)
{
  net_node_t * node;
  net_protocol_t * protocol;

  while ((node= cli_enum_net_nodes(text, state++)) != NULL) {

    // Check if node supports BGP
    protocol= node_get_protocol(node, NET_PROTOCOL_BGP);
    if (protocol == NULL)
      continue;

    return (bgp_router_t *) protocol->handler;
  }
  return NULL;
}

bgp_peer_t * cli_enum_aslevel_links(const char * text, int state)
{
  static unsigned int index= 0;
  char str_asn[ASN_STR_LEN];
  as_level_link_t * link;

  if (_ctx_as == NULL)
    return NULL;

  if (state == 0)
    index= 0;

  while (index < aslevel_as_get_num_links(_ctx_as)) {
    link= aslevel_as_get_link_by_index(_ctx_as, index++);
    assert(snprintf(str_asn, ASN_STR_LEN, "%u", link->neighbor->asn) >= 0);
    index++;
    if (!strncmp(text, str_asn, strlen(text)))
      return link->peer;
  }

  return NULL;
}

// -----[ cli_enum_bgp_peers ]---------------------------------------
bgp_peer_t * cli_enum_bgp_peers(const char * text, int state)
{
  static unsigned int index= 0;

  if (state == 0)
    index= 0;

  if (_ctx_bgp_router == NULL)
    return NULL;

  assert(index >= 0);

  if (index >= bgp_peers_size(_ctx_bgp_router->peers))
    return NULL;
  return bgp_peers_at(_ctx_bgp_router->peers, index++);
}

// -----[ cli_enum_net_nodes_addr ]----------------------------------
/**
 * Enumerate all the nodes.
 */
char * cli_enum_net_nodes_addr(const char * text, int state)
{
  net_node_t * node= NULL;
  char str_addr[IP4_ADDR_STR_LEN];
  
  while ((node= cli_enum_net_nodes(text, state++)) != NULL) {
    assert(ip_address_to_string(node->rid, str_addr, sizeof(str_addr)) >= 0);
    return strdup(str_addr);
  }
  return NULL;
}

// -----[ cli_enum_aslevel_links_addr ]------------------------------
/**
 * Enumerate all the IP addresses of peers corresponding to a partial
 * ASN ("AS" + xxx).
 */
char * cli_enum_aslevel_links_addr(const char * text, int state)
{
  char asn_addr[ASN_STR_LEN+2];
  bgp_peer_t * peer= NULL;
  
  while ((peer= cli_enum_aslevel_links(text, state++)) != NULL) {
    if (peer == NULL)
      continue;
    asn_addr[0]= 'A';
    asn_addr[1]= 'S';
    assert(snprintf(asn_addr+2, sizeof(asn_addr)-2, "%u", peer->asn) >= 0);
    return strdup(asn_addr);
  }
  return NULL;
}

// -----[ cli_enum_aslevel_domains_addr ]----------------------------
/**
 * Enumerate all the IP addresses of routers corresponding to a partial
 * ASN ("AS" + xxx).
 */
char * cli_enum_aslevel_domains_addr(const char * text, int state)
{
  char asn_addr[ASN_STR_LEN+2];
  as_level_domain_t * domain= NULL;
  
  while ((domain= cli_enum_as_level_domains(text, state++)) != NULL) {
    if (domain == NULL)
      continue;
    if (domain->router == NULL)
      continue;
    asn_addr[0]= 'A';
    asn_addr[1]= 'S';
    assert(snprintf(asn_addr+2, sizeof(asn_addr)-2, "%u", domain->asn) >= 0);
    return strdup(asn_addr);
  }
  return NULL;
}

// -----[ cli_enum_net_nodes_addr_id ]--------------------------------
char * cli_enum_net_nodes_addr_id(const char * text, int state)
{
  if (strncmp("AS", text, 2))
    return cli_enum_net_nodes_addr(text, state);
  return cli_enum_aslevel_domains_addr(text+2, state);
}

// -----[ cli_enum_bgp_routers_addr ]--------------------------------
/**
 * Enumerate all the BGP routers.
 */
char * cli_enum_bgp_routers_addr(const char * text, int state)
{
  bgp_router_t * router= NULL;
  char str_addr[IP4_ADDR_STR_LEN];
  
  while ((router= cli_enum_bgp_routers(text, state++)) != NULL) {
    assert(ip_address_to_string(router->node->rid, str_addr,
				sizeof(str_addr)) >= 0);
    return strdup(str_addr);
  }
  return NULL;
}

// -----[ cli_enum_bgp_routers_addr_id ]-----------------------------
char * cli_enum_bgp_routers_addr_id(const char * text, int state)
{
  if (strncmp("AS", text, 2))
    return cli_enum_bgp_routers_addr(text, state);
  return cli_enum_aslevel_domains_addr(text+2, state);
}

// -----[ cli_enum_bgp_peers_addr ]----------------------------------
/**
 * Enumerate all the BGP peers of one BGP router.
 */
char * cli_enum_bgp_peers_addr(const char * text, int state)
{
  bgp_peer_t * peer= NULL;
  char str_addr[IP4_ADDR_STR_LEN];

  while ((peer= cli_enum_bgp_peers(text, state++)) != NULL) {
    assert(ip_address_to_string(peer->addr, str_addr, sizeof(str_addr)) >= 0);

    // Optionally check if prefix matches
    if ((text != NULL) && (strncmp(text, str_addr, strlen(text))))
	continue;

    return strdup(str_addr);
  }
  return NULL;
}

// -----[ cli_enum_bgp_peers_addr_id ]-----------------------------
char * cli_enum_bgp_peers_addr_id(const char * text, int state)
{
  if (strncmp("AS", text, 2))
    return cli_enum_bgp_peers_addr(text, state);
  return cli_enum_aslevel_links_addr(text+2, state);
}

// -----[ cli_enum_net_node_ifaces_addr ]----------------------------
char * cli_enum_net_node_ifaces_addr(const char * text, int state)
{
  return NULL;
}
