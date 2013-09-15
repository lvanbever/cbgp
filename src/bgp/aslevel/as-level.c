// ==================================================================
// @(#)as-level.c
//
// Provide structures and functions to handle an AS-level topology
// with business relationships. This is the foundation for handling
// AS-level topologies inferred by Subramanian et al, by CAIDA and
// by Meulle et al.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/04/2007
// $Id: as-level.c,v 1.3 2009-08-31 09:35:07 bqu Exp $
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
#include <string.h>

#include <libgds/memory.h>
#include <libgds/stack.h>

#include <bgp/as.h>
#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/filter.h>
#include <bgp/aslevel/types.h>
#include <bgp/aslevel/util.h>
#include <bgp/aslevel/caida.h>
#include <bgp/aslevel/meulle.h>
#include <bgp/attr/path.h>
#include <bgp/record-route.h>
#include <bgp/aslevel/rexford.h>
#include <bgp/filter/filter.h>
#include <net/iface.h>
#include <net/node.h>

// ----- Load AS-level topology -----
static as_level_topo_t * _the_topo= NULL;

// ----- Tagging/filtering communities -----
/** communities used to enforce the valley-free property */
#define COMM_PROV 1
#define COMM_PEER 10

#define ASLEVEL_TOPO_FOREACH_DOMAIN(T,I,D)		\
  for (I= 0; I < ptr_array_length(T->domains) && (D= (as_level_domain_t*) T->domains->data[I]); I++)

// -----[ _aslevel_check_peer_type ]---------------------------------
static inline int _aslevel_check_peer_type(peer_type_t peer_type)
{
  if ((peer_type == ASLEVEL_PEER_TYPE_CUSTOMER) ||
      (peer_type == ASLEVEL_PEER_TYPE_PEER) ||
      (peer_type == ASLEVEL_PEER_TYPE_PROVIDER) ||
      (peer_type == ASLEVEL_PEER_TYPE_SIBLING))
    return ASLEVEL_SUCCESS;
  return ASLEVEL_ERROR_INVALID_RELATION;
}

// -----[ _aslevel_link_create ]-------------------------------------
static inline as_level_link_t *
_aslevel_link_create(as_level_domain_t * neighbor, peer_type_t peer_type)
{
  as_level_link_t * link= MALLOC(sizeof(as_level_link_t));
  link->neighbor= neighbor;
  link->peer_type= peer_type;
  return link;
}

// -----[ _aslevel_link_destroy ]------------------------------------
static inline void _aslevel_link_destroy(as_level_link_t ** ppLink)
{
  if (*ppLink != NULL) {
    FREE(*ppLink);
    *ppLink= NULL;
  }
}

// -----[ _aslevel_as_compare_link ] --------------------------------
static int _aslevel_as_compare_link(const void * item1,
				    const void * item2,
				    unsigned int elt_size)
{
  as_level_link_t * link1= *((as_level_link_t **) item1);
  as_level_link_t * link2= *((as_level_link_t **) item2);

  if (link1->neighbor->asn < link2->neighbor->asn)
    return -1;
  else if (link1->neighbor->asn > link2->neighbor->asn)
    return 1;
  else
    return 0;
}

// -----[ _aslevel_as_destroy_link ]---------------------------------
static void _aslevel_as_destroy_link(void * item, const void * ctx)
{
  as_level_link_t * link= *((as_level_link_t **) item);

  _aslevel_link_destroy(&link);
}

// -----[ _aslevel_as_create ]---------------------------------------
static inline as_level_domain_t * _aslevel_as_create(uint16_t asn)
{
  as_level_domain_t * domain= MALLOC(sizeof(as_level_domain_t));
  domain->asn= asn;
  domain->neighbors= ptr_array_create(ARRAY_OPTION_SORTED |
				      ARRAY_OPTION_UNIQUE,
				      _aslevel_as_compare_link,
				      _aslevel_as_destroy_link,
				      NULL);
  domain->router= NULL;
  return domain;
}

// -----[ _aslevel_as_destroy ]--------------------------------------
static inline void _aslevel_as_destroy(as_level_domain_t ** domain_ref)
{
  if (*domain_ref != NULL) {
    if ((*domain_ref)->neighbors != NULL)
      ptr_array_destroy(&(*domain_ref)->neighbors);
    //stream_printf(gdserr, "free domain %u !", (*ppDomain)->asn);
    FREE(*domain_ref);
    *domain_ref= NULL;
  }
}

// -----[ _aslevel_topo_compare_as ] --------------------------------
static int _aslevel_topo_compare_as(const void * item1,
				    const void * item2,
				    unsigned int elt_size)
{
  as_level_domain_t * domain1= *((as_level_domain_t **) item1);
  as_level_domain_t * domain2= *((as_level_domain_t **) item2);

  if (domain1->asn < domain2->asn)
    return -1;
  else if (domain1->asn > domain2->asn)
    return 1;
  else
    return 0;
}

// -----[ _aslevel_topo_destroy_as ]--------------------------------
static void _aslevel_topo_destroy_as(void * item, const void * ctx)
{
  as_level_domain_t * domain= *((as_level_domain_t **) item);

  _aslevel_as_destroy(&domain);
}

// -----[ _aslevel_create_array_domains ]----------------------------
static inline ptr_array_t * _aslevel_create_array_domains(int iRef)
{
  return ptr_array_create(ARRAY_OPTION_SORTED | ARRAY_OPTION_UNIQUE,
			  _aslevel_topo_compare_as,
			  (iRef?NULL:_aslevel_topo_destroy_as),
			  NULL);
}

// -----[ aslevel_perror ]-------------------------------------------
void aslevel_perror(gds_stream_t * stream, int error)
{
  char * error_msg= aslevel_strerror(error);
  if (error_msg != NULL)
    stream_printf(stream, error_msg);
  else
    stream_printf(stream, "unknown error (%i)", error);
}

// -----[ aslevel_strerror ]-----------------------------------------
char * aslevel_strerror(int error)
{
  switch (error) {
  case ASLEVEL_SUCCESS:
    return "success";
  case ASLEVEL_ERROR_UNEXPECTED:
    return "unexpected error";
  case ASLEVEL_ERROR_OPEN:
    return "file open error";
  case ASLEVEL_ERROR_NUM_PARAMS:
    return "invalid number of parameters";
 case ASLEVEL_ERROR_INVALID_ASNUM:
    return "invalid ASN";
  case ASLEVEL_ERROR_INVALID_RELATION:
    return "invalid business relationship";
  case ASLEVEL_ERROR_INVALID_DELAY:
    return "invalid delay";
  case ASLEVEL_ERROR_DUPLICATE_LINK:
    return "duplicate link";
  case ASLEVEL_ERROR_LOOP_LINK:
    return "loop link";
  case ASLEVEL_ERROR_NODE_EXISTS:
    return "node already exists";
  case ASLEVEL_ERROR_NO_TOPOLOGY:
    return "no topology loaded";
  case ASLEVEL_ERROR_TOPOLOGY_LOADED:
    return "topology already loaded";
  case ASLEVEL_ERROR_UNKNOWN_FORMAT:
    return "unknown topology format";
  case ASLEVEL_ERROR_CYCLE_DETECTED:
    return "topology contains cycle(s)";
  case ASLEVEL_ERROR_DISCONNECTED:
    return "topology is not connected";
  case ASLEVEL_ERROR_INCONSISTENT:
    return "topology is not consistent";
  case ASLEVEL_ERROR_UNKNOWN_FILTER:
    return "unknown topology filter";
  case ASLEVEL_ERROR_NOT_INSTALLED:
    return "topology is not installed";
  case ASLEVEL_ERROR_ALREADY_INSTALLED:
    return "topology is already installed";
  case ASLEVEL_ERROR_ALREADY_RUNNING:
    return "topology is already running";
  }
  return NULL;
}


/////////////////////////////////////////////////////////////////////
//
// TOPOLOGY MANAGEMENT
//
/////////////////////////////////////////////////////////////////////

// -----[ _aslevel_topo_cache_init ]---------------------------------
static inline void _aslevel_topo_cache_init(as_level_topo_t * topo)
{
  unsigned int index;
  
  for (index= 0; index < ASLEVEL_TOPO_AS_CACHE_SIZE; index++)
    topo->cached_domains[index]= NULL;
}

// -----[ _aslevel_topo_cache_invalidate ]---------------------------
static inline void _aslevel_topo_cache_invalidate(as_level_topo_t * topo)
{
  _aslevel_topo_cache_init(topo);
}

// -----[ _aslevel_topo_cache_update_as ]----------------------------
static inline void _aslevel_topo_cache_update_as(as_level_topo_t * topo,
						 as_level_domain_t * domain)
{
  // Shift current entries
  if (ASLEVEL_TOPO_AS_CACHE_SIZE > 0)
    memmove(&topo->cached_domains[0], &topo->cached_domains[1],
	    sizeof(as_level_domain_t *)*(ASLEVEL_TOPO_AS_CACHE_SIZE-1));
  
  // Update last entry
  topo->cached_domains[ASLEVEL_TOPO_AS_CACHE_SIZE-1]= domain;
}

// -----[ _aslevel_topo_cache_lookup_as ]----------------------------
static inline
as_level_domain_t * _aslevel_topo_cache_lookup_as(as_level_topo_t * topo,
						  uint16_t asn)
{
  unsigned int index;
  
  // Start from end of cache. Stop as soon as an empty location or a
  // matching location is found.
  for (index= ASLEVEL_TOPO_AS_CACHE_SIZE; index > 0; index--) {
    if (topo->cached_domains[index-1] == NULL)
      return NULL;
    if (topo->cached_domains[index-1]->asn == asn)
      return topo->cached_domains[index-1];
  }
  return NULL;
}

// -----[ aslevel_topo_create ]------------------------------------
as_level_topo_t * aslevel_topo_create(uint8_t addr_scheme)
{
  as_level_topo_t * topo= MALLOC(sizeof(as_level_topo_t));
  topo->domains= _aslevel_create_array_domains(0);
  switch (addr_scheme) {
  case ASLEVEL_ADDR_SCH_DEFAULT:
    topo->addr_mapper= aslevel_addr_sch_default_get;
    break;
  case ASLEVEL_ADDR_SCH_LOCAL:
    topo->addr_mapper= aslevel_addr_sch_local_get;
    break;
  default:
    abort();
  }
  topo->state= ASLEVEL_STATE_LOADED;
  _aslevel_topo_cache_init(topo);
  return topo;
}

// -----[ aslevel_topo_destroy ]-----------------------------------
void aslevel_topo_destroy(as_level_topo_t ** topo_ref)
{
  if (*topo_ref != NULL) {
    if ((*topo_ref)->domains != NULL)
      ptr_array_destroy(&(*topo_ref)->domains);
    FREE(*topo_ref);
    *topo_ref= NULL;
  }
}

// -----[ aslevel_topo_num_nodes ]---------------------------------
unsigned int aslevel_topo_num_nodes(as_level_topo_t * topo)
{
  return ptr_array_length(topo->domains);
}

// -----[ aslevel_topo_num_edges ]---------------------------------
unsigned int aslevel_topo_num_edges(as_level_topo_t * topo)
{
  unsigned int num_edges= 0;
  unsigned int index;
  as_level_domain_t * domain;

  ASLEVEL_TOPO_FOREACH_DOMAIN(topo, index, domain) {
    num_edges+= ptr_array_length(domain->neighbors);
  }
  return num_edges/2;
}

// -----[ aslevel_topo_add_as ]------------------------------------
as_level_domain_t * aslevel_topo_add_as(as_level_topo_t * topo, uint16_t asn)
{
  as_level_domain_t * domain= _aslevel_as_create(asn);
  if (ptr_array_add(topo->domains, &domain) < 0) {
    _aslevel_as_destroy(&domain);
    return NULL;
  }

  _aslevel_topo_cache_update_as(topo, domain);
  return domain;
}

// -----[ aslevel_topo_remove_as ]-----------------------------------
int aslevel_topo_remove_as(as_level_topo_t * topo, asn_t asn)
{
  as_level_domain_t dummy_domain= { .asn= asn };
  as_level_domain_t * domain;
  as_level_link_t dummy_link;
  as_level_link_t * link, * link2;
  unsigned int index, index2, index3;

  // Full search...
  domain= &dummy_domain;
  if (ptr_array_sorted_find_index(topo->domains, &domain, &index) == 0) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    _aslevel_topo_cache_invalidate(topo);

    // Remove links from neighbor domains to this domain
    for (index2= 0; index2 < ptr_array_length(domain->neighbors); index2++) {
      link= (as_level_link_t *) domain->neighbors->data[index2];
      dummy_link.neighbor= domain;
      link2= &dummy_link;
      if (ptr_array_sorted_find_index(link->neighbor->neighbors, &link2, &index3) == 0) {
	ptr_array_remove_at(link->neighbor->neighbors, index3);
      } else {
	abort();
      }
    }

    // Remove domain
    ptr_array_remove_at(topo->domains, index);

    return ASLEVEL_SUCCESS;
  }

  // Not found
  return ASLEVEL_ERROR_INVALID_ASNUM;
}

// -----[ aslevel_topo_get_as ]--------------------------------------
as_level_domain_t * aslevel_topo_get_as(as_level_topo_t * topo,
					uint16_t asn)
{
  as_level_domain_t dummy_domain= { .asn= asn };
  as_level_domain_t * domain;
  unsigned int index;

  // Domain in cache ?
  domain= _aslevel_topo_cache_lookup_as(topo, asn);
  if (domain != NULL)
    return domain;

  // Full search...
  domain= &dummy_domain;
  if (ptr_array_sorted_find_index(topo->domains, &domain, &index) == 0) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    _aslevel_topo_cache_update_as(topo, domain);
    return domain;
  }

  // Not found
  return NULL;
}

// -----[ aslevel_as_num_customers ]---------------------------------
unsigned int aslevel_as_num_customers(as_level_domain_t * domain)
{
  unsigned int index;
  unsigned int num_customers= 0;
  as_level_link_t * link;

  for (index= 0; index < ptr_array_length(domain->neighbors); index++) {
    link= (as_level_link_t *) domain->neighbors->data[index];
    if (link->peer_type == ASLEVEL_PEER_TYPE_CUSTOMER)
      num_customers++;
  }  
  
  return num_customers;
}

// -----[ aslevel_as_num_providers ]---------------------------------
unsigned int aslevel_as_num_providers(as_level_domain_t * domain)
{
  unsigned int index;
  unsigned int num_providers= 0;
  as_level_link_t * link;

  for (index= 0; index < ptr_array_length(domain->neighbors); index++) {
    link= (as_level_link_t *) domain->neighbors->data[index];
    if (link->peer_type == ASLEVEL_PEER_TYPE_PROVIDER)
      num_providers++;
  }  
  
  return num_providers;
}

// -----[ aslevel_as_add_link ]--------------------------------------
int aslevel_as_add_link(as_level_domain_t * domain1,
			as_level_domain_t * domain2,
			peer_type_t peer_type,
			as_level_link_t ** link_ref)
{
  as_level_link_t * link;

  // Check peer type validity
  if (_aslevel_check_peer_type(peer_type) != ASLEVEL_SUCCESS)
    return ASLEVEL_ERROR_INVALID_RELATION;

  // Check that link endpoints are different
  if (domain1->asn == domain2->asn)
    return ASLEVEL_ERROR_LOOP_LINK;

  link= _aslevel_link_create(domain2, peer_type);

  if (ptr_array_add(domain1->neighbors, &link) < 0) {
    _aslevel_link_destroy(&link);
    return ASLEVEL_ERROR_DUPLICATE_LINK;
  }

  if (link_ref != NULL)
    *link_ref= link;

  return ASLEVEL_SUCCESS;;
}

// -----[ aslevel_as_get_link ]--------------------------------------
as_level_link_t * aslevel_as_get_link(as_level_domain_t * domain1,
				      as_level_domain_t * domain2)
{
  as_level_link_t dummy_link= { .neighbor= domain2 };
  as_level_link_t * link= &dummy_link;
  unsigned int index;

  if (domain1->neighbors == NULL)
    return NULL;

  if (ptr_array_sorted_find_index(domain1->neighbors, &link, &index) == 0)
    return (as_level_link_t *) domain1->neighbors->data[index];

  return NULL;
}

// -----[ aslevel_as_get_link_by_index ]-----------------------------
as_level_link_t * aslevel_as_get_link_by_index(as_level_domain_t * domain,
					       int index)
{
  if (index < ptr_array_length(domain->neighbors))
    return domain->neighbors->data[index];
  return NULL;
}

// -----[ aslevel_as_get_num_links ]-------------------------------
unsigned int aslevel_as_get_num_links(as_level_domain_t * domain)
{
  return ptr_array_length(domain->neighbors);
}

// -----[ aslevel_link_get_peer_type ]-------------------------------
peer_type_t aslevel_link_get_peer_type(as_level_link_t * link)
{
  return link->peer_type;
}

// -----[ aslevel_topo_get_top ]-------------------------------------
ptr_array_t * aslevel_topo_get_top(as_level_topo_t * topo)
{
  unsigned int index;
  as_level_domain_t * domain;
  ptr_array_t * array= _aslevel_create_array_domains(1);

  ASLEVEL_TOPO_FOREACH_DOMAIN(topo, index, domain) {
    if (aslevel_as_num_providers(domain) == 0)
      ptr_array_add(array, &domain);
  }
  return array;
}

typedef struct {
  as_level_domain_t * domain;
  unsigned int index;
} SGTCtx;

// -----[ _ctx_create ]----------------------------------------------
static inline SGTCtx * _ctx_create(as_level_domain_t * domain)
{
  SGTCtx * pCtx= MALLOC(sizeof(SGTCtx));
  pCtx->domain= domain;
  pCtx->index= 0;
  return pCtx;
}

// -----[ _ctx_destroy ]---------------------------------------------
static inline void _ctx_destroy(SGTCtx ** ppCtx)
{
  if (*ppCtx != NULL) {
    FREE(*ppCtx);
    *ppCtx= NULL;
  }
}

// -----[ aslevel_topo_check_connectedness ]-------------------------
/**
 * Start from one node and check that all other nodes are reachable
 * (regardless of the business relationships). This is a depth first
 * traversal since the depth of the topology is expected to be lower
 * than its width.
 */
int aslevel_topo_check_connectedness(as_level_topo_t * topo)
{
#define MAX_QUEUE_SIZE MAX_AS
  as_level_domain_t * domain;
  as_level_link_t * link;
  gds_stack_t * stack;
  SGTCtx * ctx;
  unsigned int index;
  AS_SET_DEFINE(uASVisited);

  // No nodes => connected
  if (aslevel_topo_num_nodes(topo) == 0)
    return ASLEVEL_SUCCESS;
  
  // Initialize array of visited ASes
  AS_SET_INIT(uASVisited);

  stack= stack_create(MAX_QUEUE_SIZE);

  // Start with first node
  domain= (as_level_domain_t *) topo->domains->data[0];
  stack_push(stack, _ctx_create(domain));
  AS_SET_PUT(uASVisited, domain->asn);

  // Depth-first traversal of the graph (regardless of policies).
  // Mark each AS as it is reached.
  while (!stack_is_empty(stack)) {
    ctx= (SGTCtx *) stack_top(stack);

    if (ctx->index < ptr_array_length(ctx->domain->neighbors)) {

      link= (as_level_link_t *) ctx->domain->neighbors->data[ctx->index];
      if (!IS_IN_AS_SET(uASVisited, link->neighbor->asn)) {
	AS_SET_PUT(uASVisited, link->neighbor->asn);
	assert(stack_push(stack, _ctx_create(link->neighbor)) == 0);
      }
      ctx->index++;

    } else {
      ctx= stack_pop(stack);
      _ctx_destroy(&ctx);
    }

  }

  stack_destroy(&stack);

  // Check that every domain was reached
  for (index= 0; index < aslevel_topo_num_nodes(topo); index++) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    if (!IS_IN_AS_SET(uASVisited, domain->asn))
      return ASLEVEL_ERROR_DISCONNECTED;
  }

  return ASLEVEL_SUCCESS;
#undef MAX_QUEUE_SIZE
}

// -----[ aslevel_topo_check_cycle ]---------------------------------
/**
 * Start from top nodes (those that have no provider) and go down the
 * hierarchy strictly following provider->customer edges. Check that
 * no cycle is found.
 */
int aslevel_topo_check_cycle(as_level_topo_t * topo, int verbose)
{
#define MAX_QUEUE_SIZE 100
  ptr_array_t * top_domains;
  as_level_domain_t * domain, * cycle_domain;
  as_level_link_t * link;
  gds_stack_t * stack;
  SGTCtx * ctx;
  unsigned int uIndx, index, index2;
  int cycle;
  int num_cycles= 0;
  AS_SET_DEFINE(uASInCycle);
  unsigned int num_paths;
  unsigned int max_path_length;
  unsigned int num_reached;
  AS_SET_DEFINE(uASReached);
  AS_SET_DEFINE(uASVisited);

  // No nodes => no cycle
  if (aslevel_topo_num_nodes(topo) == 0)
    return ASLEVEL_SUCCESS;

  // Identify top domains, i.e. domains without a provider:
  //   the search will start from those providers and go down
  //   the topology along P2C relationships.
  top_domains= aslevel_topo_get_top(topo);

  if (verbose)
    stream_printf(gdsout, "number of top domains: %d\n",
		  ptr_array_length(top_domains));

  // No top domain => cycle of provider-customer relationships
  // Note: * if there is a single node, it is a top-domain.
  //       * if there is more than one node and no top-domain, there
  //         must be a cycle of P2C relationships
  if (ptr_array_length(top_domains) == 0) {
    if (verbose)
      stream_printf(gdsout, "(> 1 node) & (no top-domain) => cycle\n");
    return ASLEVEL_ERROR_CYCLE_DETECTED;
  }

  // Initialize array of visited ASes
  AS_SET_INIT(uASInCycle);

  // Starting from each top AS, traverse all the p2c edges
  cycle= 0;
  for (uIndx= 0; (uIndx < ptr_array_length(top_domains)) && !cycle; uIndx++) {
    stack= stack_create(MAX_QUEUE_SIZE);

    domain= (as_level_domain_t *) top_domains->data[uIndx];
    assert(stack_push(stack, _ctx_create(domain)) == 0);

    if (verbose)
      stream_printf(gdsout, "-> from AS%d", domain->asn);

    AS_SET_INIT(uASReached);
    AS_SET_PUT(uASReached, domain->asn);
    num_paths= 0;
    max_path_length= 0;

    AS_SET_INIT(uASVisited);
    AS_SET_PUT(uASVisited, domain->asn);

    cycle= 0;
    while (!stack_is_empty(stack) && !cycle) {
      ctx= (SGTCtx *) stack_top(stack);

      AS_SET_PUT(uASReached, ctx->domain->asn);
      if (ctx->domain == domain) {
	//fprintf(stderr, "\r-> from AS%d            ", pDomain->asn);
	if (ctx->index < ptr_array_length(ctx->domain->neighbors)) {
	  link= (as_level_link_t *) ctx->domain->neighbors->data[ctx->index];
	  if (verbose)
	    stream_printf(gdsout, "\r-> from AS%d (%d:AS%d)",
			  domain->asn, ctx->index, link->neighbor->asn);
	} else
	  if (verbose)
	    stream_printf(gdsout, "\r-> from AS%d (completed)", domain->asn);
      }

      if (ctx->index < ptr_array_length(ctx->domain->neighbors)) {
	
	link= (as_level_link_t *) ctx->domain->neighbors->data[ctx->index];
	if (!IS_IN_AS_SET(uASInCycle, link->neighbor->asn) &&
	    (link->peer_type == ASLEVEL_PEER_TYPE_CUSTOMER)) {
	  cycle= 0;

	  if (IS_IN_AS_SET(uASVisited, link->neighbor->asn)) {

	    for (index= 0; index < stack_depth(stack); index++)
	      if (((SGTCtx *) stack_get_at(stack, index))->domain == link->neighbor) {
		if (verbose)
		  stream_printf(gdsout, "\rcycle detected:");
		// Mark all ASes in the cycle
		for (index2= index; index2 < stack_depth(stack); index2++) {
		  cycle_domain= ((SGTCtx *) stack_get_at(stack, index2))->domain;
		  if (verbose)
		    stream_printf(gdsout, " %u", cycle_domain->asn);
		  AS_SET_PUT(uASInCycle, cycle_domain->asn);
		}
		if (verbose)
		  stream_printf(gdsout, " %u\n", link->neighbor->asn);
		
		cycle= 1;
		num_cycles++;
		break;
	      }
	  }

	  if (cycle == 0) {
	    assert(stack_push(stack, _ctx_create(link->neighbor)) == 0);
	    AS_SET_PUT(uASVisited, link->neighbor->asn);
	    if (stack_depth(stack) > max_path_length)
	      max_path_length= stack_depth(stack);
	  }
	  cycle= 0;
	}

	ctx->index++;
	
      } else {
	ctx= stack_pop(stack);
	AS_SET_CLEAR(uASVisited, ctx->domain->asn);
	_ctx_destroy(&ctx);
      }

    }

    if (verbose) {
      stream_printf(gdsout, "\n");

      num_reached= 0;
      for (index2= 0; index2 < MAX_AS; index2++) {
	if (IS_IN_AS_SET(uASReached, index2))
	  num_reached++;
      }
      stream_printf(gdsout, "\tnumber of paths traversed : %d\n", num_paths);
      stream_printf(gdsout, "\tmax path length           : %d\n", max_path_length);
      stream_printf(gdsout, "\tnumber of domains reached : %d\n", num_reached);
      stream_printf(gdsout, "\tnumber of direct customers: %d\n",
	      aslevel_as_num_customers(domain));
    }
      
  }

  // Clear stack (if not empty)
  while (!stack_is_empty(stack)) {
    ctx= stack_top(stack);
    _ctx_destroy(&ctx);
  }
  stack_destroy(&stack);

  ptr_array_destroy(&top_domains);

  if (num_cycles > 0)
    return ASLEVEL_ERROR_CYCLE_DETECTED;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_check_consistency ]---------------------------
/**
 * Check that all links are defined in both directions. Check that
 * both directions are tagged with matching business relationships.
 */
int aslevel_topo_check_consistency(as_level_topo_t * topo)
{
  unsigned int index, index2;
  as_level_domain_t * domain;
  as_level_link_t * link1, * link2;

  for (index= 0; index < ptr_array_length(topo->domains); index++) {
    domain= (as_level_domain_t *) topo->domains->data[index];

    for (index2= 0; index2 < ptr_array_length(domain->neighbors);
	 index2++) {
      link1= (as_level_link_t *) domain->neighbors->data[index2];

      // Check existence of reverse direction
      link2= aslevel_as_get_link(link1->neighbor, domain);
      if (link2 == NULL)
	return ASLEVEL_ERROR_INCONSISTENT;

      // Check coherence of business relationships
      if (link1->peer_type != aslevel_reverse_relation(link2->peer_type))
	return ASLEVEL_ERROR_INCONSISTENT;
    }
  }

  return ASLEVEL_SUCCESS;
}

// -----[ _aslevel_build_bgp_router ]--------------------------------
static inline bgp_router_t * _aslevel_build_bgp_router(net_addr_t addr,
						       uint16_t asn)
{
  net_node_t * node;
  bgp_router_t * router= NULL;
  net_protocol_t * protocol;
  net_error_t error;

  node= network_find_node(network_get_default(), addr);
  if (node == NULL) {
    error= node_create(addr, &node, NODE_OPTIONS_LOOPBACK);
    if (error != ESUCCESS)
      return NULL;
    error= bgp_add_router(asn, node, &router);
    if (error != ESUCCESS)
      return NULL;
    error= network_add_node(network_get_default(), node);
    if (error != ESUCCESS)
      return NULL;
  } else {
    protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
    if (protocol != NULL)
      router= (bgp_router_t *) protocol->handler;
  }
  return router;
}

// -----[ _aslevel_build_link ]--------------------------------------
static inline int
_aslevel_build_link(bgp_router_t * router1, bgp_router_t * router2)
{
  net_node_t * node1= router1->node;
  net_node_t * node2= router2->node;
  ip_pfx_t prefix;
  net_iface_t * iface;
  igp_weight_t weight= 0;
  int result;

  // Add the link and check if it did not already exist
  result= net_link_create_rtr(node1, node2, BIDIR, &iface);
  if (result != ESUCCESS)
    return result;

  // Configure IGP weight in forward direction
  assert(iface != NULL);
  assert(net_iface_set_metric(iface, 0, weight, BIDIR) == ESUCCESS);
      
  // Add static route in forward direction
  prefix.network= node2->rid;
  prefix.mask= 32;
  assert(!node_rt_add_route(node1, prefix, net_iface_id_addr(node2->rid),
			    node2->rid, weight, NET_ROUTE_STATIC));

  // Add static route in reverse direction
  prefix.network= node1->rid;
  prefix.mask= 32;
  assert(!node_rt_add_route(node2, prefix, net_iface_id_addr(node1->rid),
			    node1->rid, weight, NET_ROUTE_STATIC));
  return 0;
}

// -----[ _aslevel_build_bgp_session ]-------------------------------
static inline int
_aslevel_build_bgp_session(bgp_router_t * router1,
			   bgp_router_t * router2,
			   bgp_peer_t ** peer)
{
  net_node_t * node2= router2->node;
  
  // Setup peering relations in bothsingle direction
  if (bgp_router_add_peer(router1, router2->asn, node2->rid, peer) != 0)
    return -1;

  return 0;
}

// -----[ aslevel_topo_build_network ]-------------------------------
int aslevel_topo_build_network(as_level_topo_t * topo)
{
  unsigned int index, index2;
  as_level_domain_t * domain1, * domain2;
  as_level_link_t * neighbor;
  net_addr_t addr;
  bgp_router_t * router;

  if (topo->state != ASLEVEL_STATE_LOADED)
    return ASLEVEL_ERROR_ALREADY_INSTALLED;

  // ****************************************************
  // *   Check that none of the routers already exist   *
  // ****************************************************
  for (index= 0; index < ptr_array_length(topo->domains); index++) {
    domain1= (as_level_domain_t *) topo->domains->data[index];

    // Determine node addresses (identifiers)
    addr= topo->addr_mapper(domain1->asn);

    // Check that node doesn't exist
    if (network_find_node(network_get_default(), addr) != NULL)
      return ASLEVEL_ERROR_NODE_EXISTS;
  }

  // ********************************
  // *   Create all nodes/routers   *
  // ********************************
  for (index= 0; index < ptr_array_length(topo->domains); index++) {
    domain1= (as_level_domain_t *) topo->domains->data[index];

    // Determine node addresses (identifiers)
    addr= topo->addr_mapper(domain1->asn);

    // Find/create AS1's node
    router= _aslevel_build_bgp_router(addr, domain1->asn);
    if (router == NULL)
      return ASLEVEL_ERROR_UNEXPECTED;

    domain1->router= router;
  }

  // *********************************
  // *   Create all links/sessions   *
  // *********************************
  for (index= 0; index < ptr_array_length(topo->domains); index++) {
    domain1= (as_level_domain_t *) topo->domains->data[index];

    for (index2= 0; index2 < ptr_array_length(domain1->neighbors);
	 index2++) {
      neighbor= (as_level_link_t *) domain1->neighbors->data[index2];
      domain2= neighbor->neighbor;

      if (domain1->asn < domain2->asn)
	if (_aslevel_build_link(domain1->router, domain2->router) != 0)
	  return ASLEVEL_ERROR_UNEXPECTED;

      if (_aslevel_build_bgp_session(domain1->router,
				     domain2->router,
				     &neighbor->peer) != 0)
	return ASLEVEL_ERROR_UNEXPECTED;

    }
  }

  topo->state= ASLEVEL_STATE_INSTALLED;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_setup_policies ]------------------------------
int aslevel_topo_setup_policies(as_level_topo_t * topo)
{
  unsigned int index, index2;
  as_level_domain_t * domain, * neighbor;
  as_level_link_t * link;
  bgp_filter_t * filter;

  if (topo->state < ASLEVEL_STATE_INSTALLED)
    return ASLEVEL_ERROR_NOT_INSTALLED;
  if (topo->state > ASLEVEL_STATE_POLICIES)
    return ASLEVEL_ERROR_ALREADY_RUNNING;

  for (index= 0; index < ptr_array_length(topo->domains); index++) {
    domain= (as_level_domain_t *) topo->domains->data[index];

    for (index2= 0; index2 < ptr_array_length(domain->neighbors);
	 index2++) {
      link= (as_level_link_t *) domain->neighbors->data[index2];
      neighbor= link->neighbor;

      filter= aslevel_filter_in(link->peer_type);
      bgp_router_peer_set_filter(domain->router,
				 neighbor->router->node->rid,
				 FILTER_IN, filter);
      filter= aslevel_filter_out(link->peer_type);
      bgp_router_peer_set_filter(domain->router,
				 neighbor->router->node->rid,
				 FILTER_OUT, filter);

    }
  }

  topo->state= ASLEVEL_STATE_POLICIES;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_load ]----------------------------------------
int aslevel_topo_load(const char * filename, uint8_t format,
		      uint8_t addr_scheme, int * line_number)
{
  FILE * file;
  int result;
  int line_num= -1;

  if (_the_topo != NULL)
    return ASLEVEL_ERROR_TOPOLOGY_LOADED;

  if ((file= fopen(filename, "r")) == NULL)
    return ASLEVEL_ERROR_OPEN;

  _the_topo= aslevel_topo_create(addr_scheme);

  // Parse input file
  switch (format) {
  case ASLEVEL_FORMAT_REXFORD:
    result= rexford_parser(file, _the_topo, &line_num);
    break;
  case ASLEVEL_FORMAT_CAIDA:
    result= caida_parser(file, _the_topo, &line_num);
    break;
  case ASLEVEL_FORMAT_MEULLE:
    result= meulle_parser(file, _the_topo, &line_num);
    break;
  default:
    result= ASLEVEL_ERROR_UNKNOWN_FORMAT;
  }

  if (line_number != NULL)
    *line_number= line_num;

  if (result != 0) {
    aslevel_topo_destroy(&_the_topo);
    return result;
  }

  fclose(file);
  return aslevel_topo_check_consistency(_the_topo);
}

// -----[ aslevel_topo_install ]-------------------------------------
int aslevel_topo_install()
{
  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  return aslevel_topo_build_network(_the_topo);
}

// -----[ aslevel_topo_check ]---------------------------------------
int aslevel_topo_check(verbose)
{
  int result;

  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  if ((result= aslevel_topo_check_connectedness(_the_topo))
      != ASLEVEL_SUCCESS)
    return result;
  
  if ((result= aslevel_topo_check_cycle(_the_topo, verbose))
      != ASLEVEL_SUCCESS)
    return result;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_run ]-----------------------------------------
int aslevel_topo_run()
{
  unsigned int index;
  as_level_domain_t * domain;

  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  if (_the_topo->state < ASLEVEL_STATE_INSTALLED)
    return ASLEVEL_ERROR_NOT_INSTALLED;

  for (index= 0; index < ptr_array_length(_the_topo->domains); index++) {
    domain= (as_level_domain_t *) _the_topo->domains->data[index];
    if (bgp_router_start(domain->router) != 0)
      return ASLEVEL_ERROR_UNEXPECTED;
  }

  _the_topo->state= ASLEVEL_STATE_RUNNING;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_policies ]------------------------------------
int aslevel_topo_policies()
{
  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  return aslevel_topo_setup_policies(_the_topo);
}

// -----[ aslevel_topo_info ]----------------------------------------
int aslevel_topo_info(gds_stream_t * stream)
{
  unsigned int num_edges= 0;
  unsigned int num_P2P= 0, num_P2C= 0, num_S2S= 0, num_unknown= 0;
  unsigned int index, index2;
  as_level_domain_t * domain;

  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  stream_printf(stream, "Number of nodes: %u\n",
	     ptr_array_length(_the_topo->domains));
  ASLEVEL_TOPO_FOREACH_DOMAIN(_the_topo, index, domain) {
    for (index2= 0; index2 < ptr_array_length(domain->neighbors); index2++) {
      num_edges++;
      switch (((as_level_link_t *) domain->neighbors->data[index2])->peer_type) {
      case ASLEVEL_PEER_TYPE_CUSTOMER:
      case ASLEVEL_PEER_TYPE_PROVIDER:
	num_P2C++;
	break;
      case ASLEVEL_PEER_TYPE_PEER:
	num_P2P++;
	break;
      case ASLEVEL_PEER_TYPE_SIBLING:
	num_S2S++;
	break;
      default:
	num_unknown++;
      }
    }
  }
  num_edges/= 2;
  num_P2C/= 2;
  num_P2P/= 2;
  num_S2S/= 2;

  stream_printf(stream, "Number of edges: %u\n", num_edges);
  stream_printf(stream, "  (p2c:%d / p2p:%d / s2s: %d)\n",
		num_P2C, num_P2P, num_S2S);
  stream_printf(stream, "Number of unknown relationships: %u\n", num_unknown);
  
  stream_printf(stream, "State          : %u\n", _the_topo->state);
  
  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_filter_as ]-----------------------------------
/**
 * This function removes the domain with a given ASN from the
 * AS-level topology.
 *
 * The topology must be in state LOADED.
 */
int aslevel_topo_filter_as(asn_t asn)
{
  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  if (_the_topo->state > ASLEVEL_STATE_LOADED)
    return ASLEVEL_ERROR_ALREADY_INSTALLED;

  return aslevel_topo_remove_as(_the_topo, asn);
}

// -----[ aslevel_topo_filter_group ]--------------------------------
/**
 * This function filters some domains/links from the AS-level
 * topology. Current filters are supported:
 * - stubs             
 * - single-homed-stubs
 * - peer-peer
 * - keep-top
 *
 * The topology must be in state LOADED
 */
int aslevel_topo_filter_group(uint8_t filter)
{
  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  if (_the_topo->state > ASLEVEL_STATE_LOADED)
    return ASLEVEL_ERROR_ALREADY_INSTALLED;

  return aslevel_filter_topo(_the_topo, filter);
}

// -----[ aslevel_topo_dump ]----------------------------------------
int aslevel_topo_dump(gds_stream_t * stream, int verbose)
{
  unsigned int index, index2;
  as_level_domain_t * domain;
  as_level_link_t * link;

  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  for (index= 0; index < aslevel_topo_num_nodes(_the_topo); index++) {
    domain= (as_level_domain_t *) _the_topo->domains->data[index];
    for (index2= 0; index2 < ptr_array_length(domain->neighbors); index2++) {
      link= (as_level_link_t *) domain->neighbors->data[index2];
      if (domain->asn < link->neighbor->asn) {
	stream_printf(stream, "%u\t%u\t%d", domain->asn, link->neighbor->asn,
		      link->peer_type);
	if (verbose)
	  stream_printf(stream, "\t%s", aslevel_peer_type2str(link->peer_type));
	stream_printf(stream, "\n");
      }
    }
  }

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_dump_graphviz ]-------------------------------
int aslevel_topo_dump_graphviz(gds_stream_t * stream)
{
  unsigned int index, index2;
  as_level_domain_t * domain;
  as_level_link_t * link;

  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  stream_printf(stream, "/**\n");
  stream_printf(stream, " * AS-level topology graphviz dot file\n");
  stream_printf(stream, " * generated by C-BGP\n");
  stream_printf(stream, " *\n");
  stream_printf(stream, " * Render with: neato -Tps <file.dot> <file.ps>\n");
  stream_printf(stream, " */\n");
  stream_printf(stream, "digraph G {\n");
  stream_printf(stream, "  overlap=scale\n");

  for (index= 0; index < aslevel_topo_num_nodes(_the_topo); index++) {
    domain= (as_level_domain_t *) _the_topo->domains->data[index];
    stream_printf(stream, "  \"");
    stream_printf(stream, "AS%u", domain->asn);
    stream_printf(stream, "\" [");
    stream_printf(stream, "shape=ellipse");
    stream_printf(stream, "] ;\n");
  }

  for (index= 0; index < aslevel_topo_num_nodes(_the_topo); index++) {
    domain= (as_level_domain_t *) _the_topo->domains->data[index];
    for (index2= 0; index2 < ptr_array_length(domain->neighbors); index2++) {
      link= (as_level_link_t *) domain->neighbors->data[index2];
      if (domain->asn < link->neighbor->asn) {
	stream_printf(stream, "  \"AS%u\" ", domain->asn);
	stream_printf(stream, "->");
	stream_printf(stream, " \"AS%u\" ", link->neighbor->asn);
	stream_printf(stream, "[ ");
	switch (link->peer_type) {
	case ASLEVEL_PEER_TYPE_PEER:
	  stream_printf(stream, "dir=both,arrowhead=none,arrowtail=none");
	  break;
	case ASLEVEL_PEER_TYPE_CUSTOMER:
	  break;
	case ASLEVEL_PEER_TYPE_PROVIDER:
	  stream_printf(stream, "dir=back");
	  break;
	case ASLEVEL_PEER_TYPE_SIBLING:
	  stream_printf(stream, "dir=none,arrowhead=none,arrowtail=none,style=dashed");
	  break;
	}
	stream_printf(stream, "] ;\n");
      }
    }
  }

  stream_printf(stream, "}\n");

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_dump_stubs ]----------------------------------
int aslevel_topo_dump_stubs(gds_stream_t * stream)
{
  unsigned int index, index2;
  unsigned int num_customers= 0;
  as_level_domain_t * domain;
  as_level_link_t * link;

  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  // Identify domains to be removed
  for (index= 0; index < aslevel_topo_num_nodes(_the_topo); index++) {
    domain= (as_level_domain_t *) _the_topo->domains->data[index];
    num_customers= aslevel_as_num_customers(domain);
    if (num_customers == 0) {
      stream_printf(stream, "%u", domain->asn);
      for (index2= 0; index2 < ptr_array_length(domain->neighbors);
	   index2++) {
	link= (as_level_link_t *) domain->neighbors->data[index2];
	stream_printf(stream, " %u", link->neighbor->asn);
      }
      stream_printf(stream, "\n");
    }
  }

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_get_topo ]-----------------------------------------
as_level_topo_t * aslevel_get_topo()
{
  return _the_topo;
}


// -----[ aslevel_topo_record_route ]--------------------------------
int aslevel_topo_record_route(gds_stream_t * stream,
			      ip_pfx_t prefix, uint8_t options)
{
  unsigned int index;
  int result;
  bgp_path_t * path= NULL;
  as_level_domain_t * domain;

  if (_the_topo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  if (_the_topo->state < ASLEVEL_STATE_INSTALLED)
    return ASLEVEL_ERROR_NOT_INSTALLED;

  for (index= 0; index < ptr_array_length(_the_topo->domains); index++) {
    domain= (as_level_domain_t *) _the_topo->domains->data[index];
    result= bgp_record_route(domain->router, prefix, &path, options);
    bgp_dump_recorded_route(stream, domain->router, prefix,
			    path, result);
    path_destroy(&path);
  }

  return 0;
}

// -----[ aslevel_topo_str2format ]----------------------------------
int aslevel_topo_str2format(const char * pcFormat, uint8_t * puFormat)
{
  if (!strcmp(pcFormat, "default") || !strcmp(pcFormat, "rexford")) {
    *puFormat= ASLEVEL_FORMAT_REXFORD;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(pcFormat, "caida")) {
    *puFormat= ASLEVEL_FORMAT_CAIDA;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(pcFormat, "meulle")) {
    *puFormat= ASLEVEL_FORMAT_MEULLE;
    return ASLEVEL_SUCCESS;
  }
  return ASLEVEL_ERROR_UNKNOWN_FORMAT;
}


/////////////////////////////////////////////////////////////////////
//
// FILTERS MANAGEMENT
//
/////////////////////////////////////////////////////////////////////

// -----[ aslevel_str2peer_type ]------------------------------------
peer_type_t aslevel_str2peer_type(const char * pcStr)
{
  if (!strcmp(pcStr, "customer")) {
    return ASLEVEL_PEER_TYPE_CUSTOMER;
  } else if (!strcmp(pcStr, "peer")) {
    return ASLEVEL_PEER_TYPE_PEER;
  } else if (!strcmp(pcStr, "provider")) {
    return ASLEVEL_PEER_TYPE_PROVIDER;
  } else if (!strcmp(pcStr, "sibling")) {
    return ASLEVEL_PEER_TYPE_SIBLING;
  } 
  return ASLEVEL_PEER_TYPE_UNKNOWN;
}

// -----[ aslevel_peer_type2str ]----------------------------------
const char * aslevel_peer_type2str(peer_type_t peer_type)
{
  switch (peer_type) {
  case ASLEVEL_PEER_TYPE_CUSTOMER:
    return "customer";
  case ASLEVEL_PEER_TYPE_PEER:
    return "peer";
  case ASLEVEL_PEER_TYPE_PROVIDER:
    return "provider";
  case ASLEVEL_PEER_TYPE_SIBLING:
    return "sibling";
  default:
    return "unknown";
  }
}


// -----[ aslevel_reverse_relation ]---------------------------------
peer_type_t aslevel_reverse_relation(peer_type_t tPeerType)
{
  switch (tPeerType) {
  case ASLEVEL_PEER_TYPE_CUSTOMER:
    return ASLEVEL_PEER_TYPE_PROVIDER;
  case ASLEVEL_PEER_TYPE_PROVIDER:
    return ASLEVEL_PEER_TYPE_CUSTOMER;
  case ASLEVEL_PEER_TYPE_PEER:
    return ASLEVEL_PEER_TYPE_PEER;
  case ASLEVEL_PEER_TYPE_SIBLING:
    return ASLEVEL_PEER_TYPE_SIBLING;
  }
  return ASLEVEL_PEER_TYPE_UNKNOWN;
}

// -----[ aslevel_filter_in ]----------------------------------------
/**
 * Create an input filter based on the peer type.
 */
bgp_filter_t * aslevel_filter_in(peer_type_t tPeerType)
{
  bgp_filter_t * pFilter= NULL;

  switch (tPeerType) {
  case ASLEVEL_PEER_TYPE_CUSTOMER:
    pFilter= filter_create();
    filter_add_rule(pFilter, NULL, filter_action_pref_set(ASLEVEL_PREF_CUST));
    break;
  case ASLEVEL_PEER_TYPE_PROVIDER:
    pFilter= filter_create();
    filter_add_rule(pFilter, NULL, filter_action_comm_append(COMM_PROV));
    filter_add_rule(pFilter, NULL, filter_action_pref_set(ASLEVEL_PREF_PROV));
    break;
  case ASLEVEL_PEER_TYPE_PEER:
    pFilter= filter_create();
    filter_add_rule(pFilter, NULL, filter_action_comm_append(COMM_PEER));
    filter_add_rule(pFilter, NULL, filter_action_pref_set(ASLEVEL_PREF_PEER));
    break;
  case ASLEVEL_PEER_TYPE_SIBLING:
    break;
  }
  return pFilter;
}

// -----[ aslevel_filter_out ]---------------------------------------
/**
 * Create an output filter based on the peer type.
 */
bgp_filter_t * aslevel_filter_out(peer_type_t tPeerType)
{
  bgp_filter_t * pFilter= NULL;

  switch (tPeerType) {
  case ASLEVEL_PEER_TYPE_CUSTOMER:
    pFilter= filter_create();
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PROV));
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PEER));
    break;
  case ASLEVEL_PEER_TYPE_PROVIDER:
    pFilter= filter_create();
    filter_add_rule(pFilter, FTM_OR(filter_match_comm_contains(COMM_PROV),
				    filter_match_comm_contains(COMM_PEER)),
		    FTA_DENY);
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PROV));
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PEER));
    break;
  case ASLEVEL_PEER_TYPE_PEER:
    pFilter= filter_create();
    filter_add_rule(pFilter,
		    FTM_OR(filter_match_comm_contains(COMM_PROV),
			   filter_match_comm_contains(COMM_PEER)),
		    FTA_DENY);
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PROV));
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PEER));
    break;
  case ASLEVEL_PEER_TYPE_SIBLING:
    break;
  }
  return pFilter;
}


/////////////////////////////////////////////////////////////////////
//
// ADDRESSING SCHEMES MANAGEMENT
//
/////////////////////////////////////////////////////////////////////

// -----[ aslevel_str2addr_sch ]-------------------------------------
int aslevel_str2addr_sch(const char * pcStr, uint8_t * puAddrScheme)
{
  if (!strcmp(pcStr, "global") || !strcmp(pcStr, "default")) {
    *puAddrScheme= ASLEVEL_ADDR_SCH_GLOBAL;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(pcStr, "local")) {
    *puAddrScheme= ASLEVEL_ADDR_SCH_LOCAL;
    return ASLEVEL_SUCCESS;
  }
  return ASLEVEL_ERROR_UNKNOWN_ADDRSCH;
}

// -----[ aslevel_addr_sch_default_get ]----------------------------
/**
 * Return an IP address in the local prefix 0.0.0.0/8. The address is
 * derived from the AS number as follows:
 *
 *   addr = ASNH.ASNL.0.0
 *
 * where ASNH is composed of the 8 most significant bits of the ASN
 * while ASNL is composed of the 8 less significant bits of the ASN.
 */
net_addr_t aslevel_addr_sch_default_get(uint16_t asn)
{
  return (asn << 16);
}

// -----[ aslevel_addr_sch_local_get ]-------------------------------
/**
 * Return an IP address in the local prefix 0.0.0.0/8. The address is
 * derived from the AS number as follows:
 *
 *   addr = 0.0.ASNH.ASNL
 *
 * where ASNH is composed of the 8 most significant bits of the ASN
 * while ASNL is composed of the 8 less significant bits of the ASN.
 */
net_addr_t aslevel_addr_sch_local_get(uint16_t asn)
{
  return asn;
}


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION & FINALIZATION
//
/////////////////////////////////////////////////////////////////////

// -----[ _aslevel_destroy ]-----------------------------------------
void _aslevel_destroy()
{
  if (_the_topo != NULL)
    aslevel_topo_destroy(&_the_topo);
}
