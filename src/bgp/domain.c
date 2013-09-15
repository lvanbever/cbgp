// ==================================================================
// @(#)domain.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 13/02/2002
// $Id: domain.c,v 1.11 2009-03-24 14:11:53 bqu Exp $
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
# include <config.h>
#endif

#include <assert.h>
#include <libgds/memory.h>
#include <libgds/radix-tree.h>

#include <bgp/as.h>
#include <bgp/domain.h>
#include <bgp/peer.h>
#include <net/icmp.h>
#include <net/network.h>

#define BGP_DOMAINS_MAX 65536
static bgp_domain_t * _domains[BGP_DOMAINS_MAX];

// ----- bgp_domain_create ------------------------------------------
/**
 * Create a BGP domain (an Autonomous System).
 */
bgp_domain_t * bgp_domain_create(asn_t asn)
{
  bgp_domain_t * domain= (bgp_domain_t *) MALLOC(sizeof(bgp_domain_t));
  domain->asn= asn;
  domain->name= NULL;

  /* Radix-tree with all routers. Destroy function is NULL. */
  domain->routers= radix_tree_create(32, NULL);

  return domain;
}

// ----- bgp_domain_destroy -----------------------------------------
/**
 * Destroy a BGP domain.
 */
void bgp_domain_destroy(bgp_domain_t ** domain_ref)
{
  if (*domain_ref != NULL) {
    radix_tree_destroy(&((*domain_ref)->routers));
    FREE(*domain_ref);
    *domain_ref= NULL;
  }
}

// -----[ bgp_domains_for_each ]-------------------------------------
/**
 *
 */
int bgp_domains_for_each(FBGPDomainsForEach for_each, void * ctx)
{
  unsigned int index;
  int result;

  for (index= 0; index < BGP_DOMAINS_MAX; index++) {
    if (_domains[index] != NULL) {
      result= for_each(_domains[index], ctx);
      if (result != 0)
	return result;
    }
  }
  return 0;
}

// ----- bgp_domain_add_router --------------------------------------
/**
 * Add a reference to a router in the domain.
 *
 * Preconditions:
 * - the router must be non NULL;
 * - the router must not exist in the domain. Otherwise, it will be
 *   replaced by the new one and memory leaks as well as unexpected
 *   results may occur.
 */
void bgp_domain_add_router(bgp_domain_t * domain, bgp_router_t * router)
{
  radix_tree_add(domain->routers, router->node->rid, 32, router);
  router->domain= domain;
}

// ----- bgp_domain_routers_for_each --------------------------------
/**
 * Call the given callback function for all routers registered in the
 * domain.
 */
int bgp_domain_routers_for_each(bgp_domain_t * domain,
				FRadixTreeForEach for_each,
				void * ctx)
{
  return radix_tree_for_each(domain->routers, for_each, ctx);
}

// ----- exists_bgp_domain ------------------------------------------
/**
 * Return true (1) if the domain identified by the given AS number
 * exists. Otherwise, return false (0).
 */
int exists_bgp_domain(asn_t asn)
{
  return (_domains[asn] != NULL);
}

// ----- get_bgp_domain ---------------------------------------------
/**
 * Get the reference of a domain identified by its AS number. If the
 * domain does not exist, it is created and registered.
 */
bgp_domain_t * get_bgp_domain(asn_t asn)
{
  if (_domains[asn] == NULL)
    _domains[asn]= bgp_domain_create(asn);
  return _domains[asn];
}

// ----- register_bgp_domain ----------------------------------------
/**
 * Register a new domain.
 *
 * Preconditions:
 * - the router must be non NULL;
 * - the router must not exist.
 */
void register_bgp_domain(bgp_domain_t * domain)
{
  assert(_domains[domain->asn] == NULL);
  _domains[domain->asn]= domain;
}

// -----[ _rescan_for_each ]-----------------------------------------
static int _rescan_for_each(uint32_t key, uint8_t key_len,
			    void * item, void * ctx)
{
  return bgp_router_scan_rib((bgp_router_t *) item);
}

// ----- bgp_domain_rescan ------------------------------------------
/**
 * Run the RIB rescan process in each router of the BGP domain.
 */
int bgp_domain_rescan(bgp_domain_t * domain)
{
  return bgp_domain_routers_for_each(domain,
				     _rescan_for_each,
				     NULL);
}

typedef struct {
  gds_stream_t * stream;
  ip_dest_t      dest;
  ip_opt_t     * opts;
} _record_route_ctx_t;

// -----[ _record_route_for_each ]-----------------------------------
static int _record_route_for_each(uint32_t key, uint8_t key_len,
				  void * item, void * ctx)
{
  net_node_t * node = ((bgp_router_t *)item)->node;
  _record_route_ctx_t * rr_ctx = (_record_route_ctx_t *) ctx;
  
  icmp_record_route(rr_ctx->stream, node, IP_ADDR_ANY, rr_ctx->dest,
		    255, 0, rr_ctx->opts);
  return 0;
}

// -----[ bgp_domain_record_route ]----------------------------------
/**
 *
 */
int bgp_domain_record_route(gds_stream_t * stream,
			    bgp_domain_t * domain, 
			    ip_dest_t dest,
			    ip_opt_t * opts)
{
  _record_route_ctx_t ctx=  {
    .stream = stream,
    .dest   = dest,
    .opts= opts
  };

  return bgp_domain_routers_for_each(domain,
				     _record_route_for_each,
				     &ctx);
}

// -----[ _build_router_list ]---------------------------------------
static int _build_router_list(uint32_t key, uint8_t key_len,
			      void * item, void * ctx)
{
  ptr_array_t * list= (ptr_array_t *) ctx;
  bgp_router_t * router= (bgp_router_t *) item;
  
  ptr_array_append(list, router);
  
  return 0;
}

// -----[ _bgp_domain_routers_list ]---------------------------------
static inline ptr_array_t * _bgp_domain_routers_list(bgp_domain_t * domain)
{
  ptr_array_t * list= ptr_array_create_ref(0);

  // Build list of BGP routers
  radix_tree_for_each(domain->routers, _build_router_list, list);

  return list;
}

// ----- bgp_domain_full_mesh ---------------------------------------
/**
 * Generate a full-mesh of iBGP sessions in the domain.
 */
int bgp_domain_full_mesh(bgp_domain_t * domain)
{
  unsigned int index1, index2;
  ptr_array_t * routers;
  bgp_router_t * router1, * router2;
  bgp_peer_t * peer;
  int result;

  /* Get the list of routers */
  routers= _bgp_domain_routers_list(domain);

  // Build the full-mesh of sessions
  for (index1= 0; index1 < ptr_array_length(routers); index1++) {
    router1= routers->data[index1];
    for (index2= 0; index2 < ptr_array_length(routers); index2++) {
      if (index1 == index2)
	continue;

      router2= routers->data[index2];

      // Add the peer
      result= bgp_router_add_peer(router1, domain->asn,
				  router2->node->rid, &peer);
      if (result != ESUCCESS)
	return result;

      // Set the source address of BGP messages sent from
      // this router to the RID of this router. We SHOULD
      // be using an existing interface address (the RID
      // is only an identifier !)
      bgp_peer_set_source(peer, router1->node->rid);

    }

    // Start BGP sessions
    result= bgp_router_start(router1);
    if (result != ESUCCESS)
      return result;

  }
  ptr_array_destroy(&routers);
  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// ----- _bgp__domain_init ------------------------------------------
/**
 * Initialize the array of BGP domains.
 */
void _bgp_domain_init()
{
  unsigned int index;

  for (index= 0; index < BGP_DOMAINS_MAX; index++) {
    _domains[index]= NULL;
  }
}

// ----- _bgp_domain_destroy ----------------------------------------
/**
 * Clean the array of domains: destroy the existing domains.
 */
void _bgp_domain_destroy()
{
  unsigned int index;

  for (index= 0; index < BGP_DOMAINS_MAX; index++) {
    bgp_domain_destroy(&_domains[index]);
  }
}
