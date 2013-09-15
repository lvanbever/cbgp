// ==================================================================
// @(#)igp_domain.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 5/07/2005
// $Id: igp_domain.c,v 1.10 2009-03-24 16:12:49 bqu Exp $
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
#include <libgds/stream.h>
#include <libgds/memory.h>
#include <libgds/radix-tree.h>
#include <net/igp.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/node.h>
#include <net/ospf.h>

// -----[ igp_domain_create ]----------------------------------------
igp_domain_t * igp_domain_create(uint16_t id, igp_domain_type_t type)
{
  igp_domain_t * domain= (igp_domain_t *) MALLOC(sizeof(igp_domain_t));
  domain->id= id;
  domain->name= NULL;
  domain->type= type;

  /* Radix-tree with all routers. Destroy function is NULL. */
  domain->routers= trie_create(NULL);

  return domain;
}

// -----[ igp_domain_destroy ]---------------------------------------
void igp_domain_destroy(igp_domain_t ** domain_ref)
{
  if (*domain_ref != NULL) {
    trie_destroy(&((*domain_ref)->routers));
    FREE(*domain_ref);
    *domain_ref= NULL;
  }
}

// -----[ igp_domain_add_router ]------------------------------------
int igp_domain_add_router(igp_domain_t * domain, net_node_t * node)
{
  trie_insert(domain->routers, node->rid, 32, node, 0);
  return node_igp_domain_add(node, domain->id);
}

// -----[ igp_domain_contains_router ]-------------------------------
int igp_domain_contains_router(igp_domain_t * domain, net_node_t * node)
{
  return igp_domain_contains_router_by_addr(domain, node->rid);
}

// -----[ igp_domain_contains_router_by_addr ]-----------------------
int igp_domain_contains_router_by_addr(igp_domain_t * domain, net_addr_t addr)
{
  if (trie_find_exact(domain->routers, addr, 32) == NULL)
    return 0;
  return 1;
}

// ----- igp_domain_routers_for_each --------------------------------
/**
 * Call the given callback function for all routers registered in the
 * domain.
 */
int igp_domain_routers_for_each(igp_domain_t * domain,
				FRadixTreeForEach for_each,
				void * ctx)
{
  trie_for_each(domain->routers, for_each, ctx);
  return 0;
}

// ----- _igp_domain_dump_for_each ----------------------------------
/**
 *  Helps igp_domain_dump to dump the ip address of each router 
 *  in the igp domain.
*/
static int _igp_domain_dump_for_each(uint32_t key, uint8_t key_len,
				     void * item, void * ctx)
{
  gds_stream_t * stream= (gds_stream_t *) ctx;
  net_node_t * node= (net_node_t *) item;
  
  ip_address_dump(stream, node->rid);
  stream_printf(stream, "\n");
  return 0;
}

// ----- igp_domain_dump --------------------------------------------
int igp_domain_dump(gds_stream_t * stream, igp_domain_t * domain)
{
  return trie_for_each(domain->routers,
		       _igp_domain_dump_for_each,
		       stream);
}

// ----- igp_domain_info --------------------------------------------
void igp_domain_info(gds_stream_t * stream, igp_domain_t * domain)
{
  stream_printf(stream, "model: ");
  switch (domain->type) {
  case IGP_DOMAIN_IGP: stream_printf(stream, "igp"); break;
  case IGP_DOMAIN_OSPF: stream_printf(stream, "ospf"); break;
  default:
    abort();
  }
  stream_printf(stream, "\n");
}

// -----[ igp_domain_compute ]---------------------------------------
int igp_domain_compute(igp_domain_t * domain, int keep_spt)
{
  switch (domain->type) {
  case IGP_DOMAIN_IGP:
    return igp_compute_domain(domain, keep_spt);
    break;
  case IGP_DOMAIN_OSPF:
#ifdef OSPF_SUPPORT
    if (ospf_domain_build_route(domain->id) >= 0)
      return 0;
#endif
    return -1;
  default:
    cbgp_fatal("invalid IGP domain type (%d)", domain->type);
    abort();
  }
}


/////////////////////////////////////////////////////////////////////
//
// LIST OF IGP DOMAINS
//
/////////////////////////////////////////////////////////////////////

// -----[ _igp_domains_cmp ]-----------------------------------------
static int _igp_domains_cmp(const void * item1,
			    const void * item2,
			    unsigned int size)
{
  igp_domain_t * domain1= *(igp_domain_t **) item1;
  igp_domain_t * domain2= *(igp_domain_t **) item2;

  if (domain1->id > domain2->id)
    return 1;
  else if (domain1->id < domain2->id)
    return -1;
  return 0;
}

// -----[ _igp_domains_destroy ]-------------------------------------
static void _igp_domains_destroy(void * item, const void * ctx)
{
  igp_domain_destroy((igp_domain_t **) item);
}

// -----[ igp_domains_create ]---------------------------------------
igp_domains_t * igp_domains_create()
{
  return (igp_domains_t *) ptr_array_create(ARRAY_OPTION_SORTED |
					    ARRAY_OPTION_UNIQUE,
					    _igp_domains_cmp,
					    _igp_domains_destroy,
					    NULL);
}

// -----[ igp_domains_destroy ]--------------------------------------
void igp_domains_destroy(igp_domains_t ** domains_ref)
{
  ptr_array_destroy(domains_ref);
}

// -----[ igp_domains_find ]-----------------------------------------
igp_domain_t * igp_domains_find(igp_domains_t * domains, uint16_t id)
{
  igp_domain_t domain= { .id= id };
  igp_domain_t * domain_ref= &domain;
  unsigned int index;

  if (ptr_array_sorted_find_index(domains, &domain_ref, &index) == 0)
    return (igp_domain_t *) domains->data[index];
  return NULL;
}

// -----[ igp_domains_add ]------------------------------------------
int igp_domains_add(igp_domains_t * domains, igp_domain_t * domain)
{
  return ptr_array_add(domains, &domain);
}

// -----[ igp_domains_for_each ]-------------------------------------
int igp_domains_for_each(igp_domains_t * domains,
			 FIGPDomainsForEach for_each, void * ctx)
{
  unsigned int index;
  int result;

  for (index= 0; index < ptr_array_length(domains); index++) {
    result= for_each((igp_domain_t *) domains->data[index], ctx);
    if (result != 0)
      return result;
  }
  return 0;
}

