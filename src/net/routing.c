// ==================================================================
// @(#)routing.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/02/2004
// $Id: routing.c,v 1.30 2009-08-31 09:48:28 bqu Exp $
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

#include <libgds/stream.h>
#include <libgds/memory.h>
#include <net/network.h>
#include <net/node.h>
#include <net/routing.h>
#include <ui/output.h>
#include <util/str_format.h>

//#define ROUTING_DEBUG

#ifdef ROUTING_DEBUG
static int _debug_for_each(gds_stream_t * stream, void * context,
			   char format)
{
  va_list * ap= (va_list*) context;
  net_node_t * node;
  net_addr_t addr;
  net_iface_t * iface;
  rt_entry_t * entry;

  switch (format) {
  case 'a':
    addr= va_arg(*ap, net_addr_t);
    ip_address_dump(stream, addr);
    break;
  case 'e':
    entry= va_arg(*ap, rt_entry_t *);
    rt_entry_dump(stream, entry);
    break;
  case 'i':
    iface= va_arg(*ap, net_iface_t *);
    net_iface_dump_id(stream, iface);
    break;
  case 'n':
    node= va_arg(*ap, net_node_t *);
    node_dump_id(stream, node);
    break;
  }
  return 0;
}
#endif /* ROUTING_DEBUG */

#ifdef ROUTING_DEBUG
static inline void ___routing_debug(const char * format, ...)
{
  va_list ap;

  va_start(ap, format);
  stream_printf(gdsout, "RT_DBG::");
  str_format_for_each(gdsout, _debug_for_each, &ap, format);
  stream_flush(gdsout);
  va_end(ap);
}
#else
#define ___routing_debug(M...)
#endif /* ROUTING_DEBUG */



///////////////////////////////////////////////////////////////////
// RT ENTRY (rt_entry_t)
///////////////////////////////////////////////////////////////////

// -----[ rt_entry_create ]------------------------------------------
rt_entry_t * rt_entry_create(net_iface_t * oif, net_addr_t gateway)
{
  rt_entry_t * entry= (rt_entry_t *) MALLOC(sizeof(rt_entry_t));
  entry->oif= oif;
  entry->gateway= gateway;
  entry->ref_cnt= 1;
  ___routing_debug("rt_entry_create %e\n", entry);
  return entry;
}

// -----[ rt_entry_destroy ]-----------------------------------------
void rt_entry_destroy(rt_entry_t ** entry_ref)
{
  if (*entry_ref != NULL) {
    assert((*entry_ref)->ref_cnt > 0);
    (*entry_ref)->ref_cnt--;
    ___routing_debug("rt_entry_dec_ref %e\n", *entry_ref);
    if ((*entry_ref)->ref_cnt > 0)
      return;
    ___routing_debug("rt_entry_destroy %e\n", *entry_ref);
    FREE(*entry_ref);
    *entry_ref= NULL;
  }
}

// -----[ rt_entry_add_ref ]-----------------------------------------
rt_entry_t * rt_entry_add_ref(rt_entry_t * entry)
{
  entry->ref_cnt++;
  ___routing_debug("rt_entry_add_ref %e\n", entry);
  return entry;
}

// -----[ rt_entry_copy ]--------------------------------------------
rt_entry_t * rt_entry_copy(const rt_entry_t * entry)
{
  return rt_entry_create(entry->oif, entry->gateway);
}

// -----[ rt_entry_dump ]--------------------------------------------
void rt_entry_dump(gds_stream_t * stream, const rt_entry_t * entry)
{
  ip_address_dump(stream, entry->gateway);
  stream_printf(stream, "\t");
  if (entry->oif != NULL)
    ip_address_dump(stream, entry->oif->addr);
  else
    stream_printf(stream, "---");
}

// -----[ rt_entry_compare ]-----------------------------------------
int rt_entry_compare(const rt_entry_t * entry1,
		     const rt_entry_t * entry2)
{
  if (entry1->gateway > entry2->gateway)
    return 1;
  if (entry1->gateway < entry2->gateway)
    return -1;

  /* At this stage, the gateway is common for both entries.
     We are going to compare based on the address of the outgoing
     interface.
     However, it is possible that one of the entries has an
     unspecified outgoing interface (i.e. NULL). In this case, 
     the unspecified interface wins (i.e. appears first). */
  if (entry1->oif == NULL)
    return 1;
  if (entry2->oif == NULL)
    return -1;

  /* At this stage, both entries have valid outgoing interfaces,
     we can compare their addresses. */
  if (entry1->oif->addr > entry2->oif->addr)
    return 1;
  if (entry1->oif->addr < entry2->oif->addr)
    return -1;

  return 0;
}


/////////////////////////////////////////////////////////////////////
// RT ENTRIES (rt_entries_t)
/////////////////////////////////////////////////////////////////////

// -----[ _rt_entries_cmp ]------------------------------------------
static int _rt_entries_cmp(const void * item1,
			   const void * item2,
			   unsigned int size)
{
  rt_entry_t * entry1= *(rt_entry_t **) item1;
  rt_entry_t * entry2= *(rt_entry_t **) item2;

  return rt_entry_compare(entry1, entry2);
}

// -----[ _rt_entries_destroy ]--------------------------------------
static void _rt_entries_destroy(void * item, const void * ctx)
{
  rt_entry_destroy((rt_entry_t **) item);
}

// -----[ rt_entries_create ]----------------------------------------
rt_entries_t * rt_entries_create()
{
  return (rt_entries_t *) ptr_array_create(ARRAY_OPTION_SORTED |
					   ARRAY_OPTION_UNIQUE,
					   _rt_entries_cmp,
					   _rt_entries_destroy,
					   NULL);
}

// -----[ rt_entries_destroy ]---------------------------------------
void rt_entries_destroy(rt_entries_t ** entries)
{
  ptr_array_destroy(entries);
}

// -----[ rt_entries_contains ]--------------------------------------
int rt_entries_contains(const rt_entries_t * entries, rt_entry_t * entry)
{
  unsigned int index;
  return (ptr_array_sorted_find_index(entries, &entry, &index) >= 0);
}

// -----[ rt_entries_add ]-------------------------------------------
int rt_entries_add(const rt_entries_t * entries, rt_entry_t * entry)
{
  return (ptr_array_add(entries, &entry) >= 0)?ESUCCESS:ENET_RT_DUPLICATE;
}

// -----[ rt_entries_del ]-------------------------------------------
int rt_entries_del(const rt_entries_t * entries, rt_entry_t * entry)
{
  unsigned int index;
  if (ptr_array_sorted_find_index(entries, &entry, &index) < 0)
    return -1;
  return ptr_array_remove_at(entries, index);
}

// -----[ rt_entries_size ]------------------------------------------
unsigned int rt_entries_size(const rt_entries_t * entries)
{
  return ptr_array_length(entries);
}

// -----[ rt_entries_get_at ]----------------------------------------
rt_entry_t * rt_entries_get_at(const rt_entries_t * entries,
			       unsigned int index)
{
  return (rt_entry_t *) entries->data[index];
}

// -----[ rt_entries_remove_at ]-------------------------------------
int rt_entries_remove_at(const rt_entries_t * entries, unsigned int index)
{
  return ptr_array_remove_at(entries, index);
}

// -----[ rt_entries_copy ]------------------------------------------
rt_entries_t * rt_entries_copy(const rt_entries_t * entries)
{
  unsigned int index;
  rt_entries_t * new_entries;

  new_entries= rt_entries_create();
  for (index= 0; index < rt_entries_size(entries); index++) {
    rt_entries_add(new_entries,
		   rt_entry_copy(rt_entries_get_at(entries, index)));
  }
  return new_entries;
}

// -----[ rt_entries_dump ]------------------------------------------
void rt_entries_dump(gds_stream_t * stream, const rt_entries_t * entries)
{
  unsigned int index;
  for (index= 0; index < rt_entries_size(entries); index++) {
    rt_entry_dump(stream, rt_entries_get_at(entries, index));
    stream_printf(stream, "\n");
  }
}


/////////////////////////////////////////////////////////////////////
//
// ROUTE (rt_info_t)
//
/////////////////////////////////////////////////////////////////////

// -----[ rt_info_create ]-------------------------------------------
rt_info_t * rt_info_create(ip_pfx_t prefix,
			   uint32_t metric,
			   net_route_type_t type)
{
  rt_info_t * rtinfo= (rt_info_t *) MALLOC(sizeof(rt_info_t));
  rtinfo->prefix= prefix;
  rtinfo->entries= rt_entries_create();
  rtinfo->metric= metric;
  rtinfo->type= type;
  return rtinfo;
}

// -----[ rt_info_add_entry ]----------------------------------------
int rt_info_add_entry(rt_info_t * rtinfo,
		      net_iface_t * oif, net_addr_t gateway)
{
  rt_entry_t * entry= rt_entry_create(oif, gateway);
  int result= rt_entries_add(rtinfo->entries, entry);
  if (result < 0)
    rt_entry_destroy(&entry);
  return result;
}

// -----[ rt_info_set_entries ]--------------------------------------
int rt_info_set_entries(rt_info_t * rtinfo, rt_entries_t * entries)
{
  rt_entries_destroy(&rtinfo->entries);
  rtinfo->entries= entries;
  return ESUCCESS;
}

// -----[ rt_info_destroy ]------------------------------------------
void rt_info_destroy(rt_info_t ** rtinfo_ref)
{
  if (*rtinfo_ref != NULL) {
    rt_entries_destroy(&(*rtinfo_ref)->entries);
    FREE(*rtinfo_ref);
    *rtinfo_ref= NULL;
  }
}

// ----- _rt_info_list_cmp ------------------------------------------
/**
 * Compare two routes towards the same destination
 * - prefer the route with the lowest type (STATIC > IGP > BGP)
 * - prefer the route with the lowest metric
 */
static int _rt_info_list_cmp(const void * item1,
			     const void * item2,
			     unsigned int elt_size)
{
  rt_info_t * rtinfo1= *((rt_info_t **) item1);
  rt_info_t * rtinfo2= *((rt_info_t **) item2);

  // Prefer lowest routing protocol type STATIC > IGP > BGP
  if (rtinfo1->type > rtinfo2->type)
    return 1;
  if (rtinfo1->type < rtinfo2->type)
    return -1;

  // Prefer route with lowest metric
  if (rtinfo1->metric > rtinfo2->metric)
    return 1;
  if (rtinfo1->metric < rtinfo2->metric)
    return -1;

  return 0;
}

// ----- _rt_info_list_dst ------------------------------------------
static void _rt_info_list_dst(void * item, const void * ctx)
{
  rt_info_t * rtinfo= *((rt_info_t **) item);
  rt_info_destroy(&rtinfo);
}

// -----[ _rt_info_list_create ]-------------------------------------
static inline rt_infos_t * _rt_info_list_create()
{
  return (rt_infos_t *) ptr_array_create(ARRAY_OPTION_SORTED|
					     ARRAY_OPTION_UNIQUE,
					     _rt_info_list_cmp,
					     _rt_info_list_dst,
					     NULL);
}

// -----[ _rt_info_list_destroy ]------------------------------------
static inline void _rt_info_list_destroy(rt_infos_t ** list_ref)
{
  ptr_array_destroy((ptr_array_t **) list_ref);
}

// -----[ _rt_info_list_length ]-------------------------------------
static inline unsigned int _rt_info_list_length(rt_infos_t * list)
{
  return ptr_array_length((ptr_array_t *) list);
}

// -----[ _rt_info_list_dump ]----------------------------------------
static inline void _rt_info_list_dump(gds_stream_t * stream,
				      ip_pfx_t prefix,
				      rt_infos_t * list)
{
  unsigned int index;

  if (_rt_info_list_length(list) == 0) {
    STREAM_ERR_ENABLED(STREAM_LEVEL_FATAL) {
      stream_printf(gdserr, "\033[1;31mERROR: empty info-list for ");
      ip_prefix_dump(gdserr, prefix);
      stream_printf(gdserr, "\033[0m\n");
    }
    abort();
  }

  for (index= 0; index < _rt_info_list_length(list); index++) {
    rt_info_dump(stream, (rt_info_t *) list->data[index]);
    stream_printf(stream, "\n");
  }
}

// -----[ _rt_info_list_add ]----------------------------------------
/**
 * Add a new route into the info-list (an info-list corresponds to a
 * destination prefix).
 *
 * Result:
 * - ESUCCESS          if the insertion succeeded
 * - ENET_RT_DUPLICATE if the route could not be added (it already
 *   exists)
 */
static inline int _rt_info_list_add(rt_infos_t * list,
				    rt_info_t * rtinfo)
{
  if (ptr_array_add((ptr_array_t *) list,
		    &rtinfo) < 0)
    return ENET_RT_DUPLICATE;
  return ESUCCESS;
}

// -----[ _net_route_info_dump_filter ]------------------------------
/**
 * Dump a route-info filter.
 */
/*static inline void
net_route_info_dump_filter(gds_stream_t * stream,
			   rt_filter_t * filter)
{
  // Destination filter
  stream_printf(stream, "prefix : ");
  if (filter->prefix != NULL)
    ip_prefix_dump(stream, *filter->prefix);
  else
    stream_printf(stream, "*");

  // Next-hop address (gateway)
  stream_printf(stream, ", gateway: ");
  if (filter->gateway != NULL)
    ip_address_dump(stream, *filter->gateway);
  else
    stream_printf(stream, "*");

  // Next-hop interface
  stream_printf(stream, ", iface   : ");
  if (filter->oif != NULL)
    ip_address_dump(stream, filter->oif->tIfaceAddr);
    //net_iface_dump_id(stream, filter->oif);
  else
    stream_printf(stream, "*");

  // Route type (routing protocol)
  stream_printf(stream, ", type: ");
  net_route_type_dump(stream, filter->type);
  stream_printf(stream, "\n");
  }*/

// -----[ _net_info_prefix_dst ]-------------------------------------
static void _net_info_prefix_dst(void * item, const void * ctx)
{
  ip_prefix_destroy((ip_pfx_t **) item);
}

// ----- net_info_schedule_removal -----------------------------------
/**
 * This function adds a prefix whose entry in the routing table must
 * be removed. This is used when a route-info-list has been emptied.
 */
static inline void
net_info_schedule_removal(rt_filter_t * filter,
			  ip_pfx_t * prefix)
{
  if (filter->del_list == NULL)
    filter->del_list= ptr_array_create(0, NULL,
				       _net_info_prefix_dst,
				       NULL);

  prefix= ip_prefix_copy(prefix);
  ptr_array_append(filter->del_list, prefix);
}

// -----[ _net_info_removal_for_each ]-------------------------------
/**
 *
 */
static int _net_info_removal_for_each(const void * item,
				      const void * ctx)
{
  net_rt_t * rt= (net_rt_t *) ctx;
  ip_pfx_t * prefix= *(ip_pfx_t **) item;

  return trie_remove(rt, prefix->network, prefix->mask);
}

// -----[ _net_info_removal ]----------------------------------------
/**
 * Remove the scheduled prefixes from the routing table.
 */
static inline int _net_info_removal(rt_filter_t * filter,
				    net_rt_t * rt)
{
  int result= 0;

  if (filter->del_list != NULL) {
    result= _array_for_each((array_t *) filter->del_list,
			     _net_info_removal_for_each, rt);
    ptr_array_destroy(&filter->del_list);
  }
  return result;
}

// -----[ _rt_info_list_del ]----------------------------------------
/**
 * This function removes from the route-list all the routes that match
 * the given attributes: next-hop and route type. A wildcard can be
 * used for the next-hop attribute (by specifying a NULL next-hop
 * link).
 */
static inline int _rt_info_list_del(rt_infos_t * list,
				    rt_filter_t * filter,
				    ip_pfx_t * prefix)
{
  unsigned int index;
  rt_info_t * rtinfo;
  unsigned int del_count= 0;
  int result= ENET_RT_UNKNOWN;

  /* Lookup the whole list of routes... */
  index= 0;
  while (index < _rt_info_list_length(list)) {
    rtinfo= (rt_info_t *) list->data[index];

    if (rt_filter_matches(rtinfo, filter)) {
      ptr_array_remove_at((ptr_array_t *) list, index);
      del_count++;
    } else {
      index++;
    }
  }
  
  /* If the list of routes does not contain any route, it should be
     destroyed. Schedule the prefix for removal... */
  if (_rt_info_list_length(list) == 0)
    net_info_schedule_removal(filter, prefix);

  if (del_count > 0)
    result= ESUCCESS;
  return result;
}

// -----[ _rt_info_list_get ]----------------------------------------
static inline rt_info_t *
_rt_info_list_get(rt_infos_t * list, unsigned int index)
{
  if (index < _rt_info_list_length(list))
    return list->data[index];
  return NULL;
}

// ----- _rt_il_dst -------------------------------------------------
static void _rt_il_dst(void ** ppItem)
{
  _rt_info_list_destroy((rt_infos_t **) ppItem);
}


/////////////////////////////////////////////////////////////////////
//
// ROUTING TABLE (net_rt_t)
//
/////////////////////////////////////////////////////////////////////

// ----- rt_create --------------------------------------------------
/**
 * Create a routing table.
 */
net_rt_t * rt_create()
{
  return (net_rt_t *) trie_create(_rt_il_dst);
}

// ----- rt_destroy -------------------------------------------------
/**
 * Free a routing table.
 */
void rt_destroy(net_rt_t ** rt_ref)
{
  trie_destroy(rt_ref);
}

// -----[ rt_find_best ]---------------------------------------------
/**
 * Find the route that best matches the given prefix. If a particular
 * route type is given, returns only the route with the requested
 * type.
 *
 * Parameters:
 * - routing table
 * - prefix
 * - route type (can be NET_ROUTE_ANY if any type is ok)
 */
rt_info_t * rt_find_best(net_rt_t * rt, net_addr_t addr,
			 net_route_type_t type)
{
  rt_infos_t * list;
  int index;
  rt_info_t * rtinfo;

  /* First, retrieve the list of routes that best match the given
     prefix */
  list= (rt_infos_t *) trie_find_best(rt, addr, 32);

  /* Then, select the first returned route that matches the given
     route-type (if requested) */
  if (list != NULL) {

    assert(_rt_info_list_length(list) != 0);

    for (index= 0; index < _rt_info_list_length(list); index++) {
      rtinfo= _rt_info_list_get(list, index);
      if (rtinfo->type & type)
	return rtinfo;
    }
  }

  return NULL;
}

// -----[ rt_find_exact ]--------------------------------------------
/**
 * Find the route that exactly matches the given prefix. If a
 * particular route type is given, returns only the route with the
 * requested type.
 *
 * Parameters:
 * - routing table
 * - prefix
 * - route type (can be NET_ROUTE_ANY if any type is ok)
 */
rt_info_t * rt_find_exact(net_rt_t * rt, ip_pfx_t prefix,
			  net_route_type_t type)
{
  rt_infos_t * list;
  int index;
  rt_info_t * rtinfo;

  /* First, retrieve the list of routes that exactly match the given
     prefix */
  list= (rt_infos_t *)
    trie_find_exact(rt, prefix.network, prefix.mask);

  /* Then, select the first returned route that matches the given
     route-type (if requested) */
  if (list != NULL) {

    assert(_rt_info_list_length(list) != 0);

    for (index= 0; index < _rt_info_list_length(list); index++) {
      rtinfo= _rt_info_list_get(list, index);
      if (rtinfo->type & type)
	return rtinfo;
    }
  }

  return NULL;
}

// -----[ rt_add_route ]---------------------------------------------
/**
 * Add a route into the routing table.
 *
 * Returns:
 *   ESUCCESS          on success
 *   ENET_RT_DUPLICATE in case of error (duplicate route)
 */
int rt_add_route(net_rt_t * rt, ip_pfx_t prefix,
		 rt_info_t * rtinfo)
{
  rt_infos_t * list;

  list= (rt_infos_t *) trie_find_exact(rt,
					   prefix.network,
					   prefix.mask);

  // Create a new info-list if none exists for the given prefix
  if (list == NULL) {

    list= _rt_info_list_create();
    assert(_rt_info_list_add(list, rtinfo) == ESUCCESS);
    trie_insert(rt, prefix.network, prefix.mask, list, 0);

  } else {

    return _rt_info_list_add(list, rtinfo);

  }
  return ESUCCESS;
}

// -----[ _rt_del_for_each ]-----------------------------------------
/**
 * Helper function for the 'rt_del_route' function. Handles the
 * deletion of a single prefix (can be called by
 * 'radix_tree_for_each')
 */
static int _rt_del_for_each(uint32_t key, uint8_t key_len,
			    void * item, void * ctx)
{
  rt_infos_t * list= (rt_infos_t *) item;
  rt_filter_t * filter= (rt_filter_t *) ctx;
  ip_pfx_t prefix;
  int result;

  if (list == NULL)
    return -1;

  prefix.network= key;
  prefix.mask= key_len;

  /* Remove from the list all the routes that match the given
     attributes */
  result= _rt_info_list_del(list, filter, &prefix);

  /* If we use wildcards, the call never fails, otherwise, it depends
     on the '_rt_info_list_del' call */
  return ((filter->prefix != NULL) &&
	  (filter->oif != NULL) &&
	  (filter->gateway != NULL)) ? result : 0;
}

// ----- rt_del_routes ----------------------------------------------
net_error_t rt_del_routes(net_rt_t * rt, rt_filter_t * filter)
{
  rt_infos_t * list;
  net_error_t error;

  /* Prefix specified ? or wildcard ? */
  if (filter->prefix != NULL) {

    /* Get the list of routes towards the given prefix */
    list= (rt_infos_t *) trie_find_exact(rt,
					     filter->prefix->network,
					     filter->prefix->mask);
    error= _rt_del_for_each(filter->prefix->network,
			    filter->prefix->mask,
			    list, filter);

  } else {

    /* Remove all the routes that match the given attributes, whatever
       the prefix is */
    error= trie_for_each(rt, _rt_del_for_each, filter);

  }

  // Post-processing, remove empty rtinfo lists
  _net_info_removal(filter, rt);

  return error;
}

// ----- rt_del_route -----------------------------------------------
/**
 * Remove route(s) from the given routing table. The route(s) to be
 * removed must match the given attributes: prefix, next-hop and
 * type. However, wildcards can be used for the prefix and next-hop
 * attributes. The NULL value corresponds to the wildcard.
 */
net_error_t rt_del_route(net_rt_t * rt, ip_pfx_t * prefix,
			 net_iface_t * oif, net_addr_t * gateway,
			 net_route_type_t type)
{
  rt_filter_t * filter= rt_filter_create();
  net_error_t error;

  /* Prepare the attributes of the routes to be removed (context) */
  filter->prefix= prefix;
  filter->oif= oif;
  filter->gateway= gateway;
  filter->type= type;

  error= rt_del_routes(rt, filter);
  rt_filter_destroy(&filter);

  return error;
}

typedef struct {
  FRadixTreeForEach   fForEach;
  void              * ctx;
} _for_each_ctx_t;

// ----- _rt_for_each_function --------------------------------------
/**
 * Helper function for the 'rt_for_each' function. This function
 * execute the callback function for each entry in the info-list of
 * the current prefix.
 */
static int _rt_for_each_function(uint32_t key, uint8_t key_len,
				 void * item, void * ctx)
{
  _for_each_ctx_t * for_each_ctx= (_for_each_ctx_t *) ctx;
  rt_infos_t * list= (rt_infos_t *) item;
  unsigned int index;

  for (index= 0; index < _rt_info_list_length(list); index++) {
    if (for_each_ctx->fForEach(key, key_len, list->data[index],
			       for_each_ctx->ctx) != 0)
      return -1;
  }
  return 0;
}

// ----- rt_for_each ------------------------------------------------
/**
 * Execute the given function for each entry (prefix, type) in the
 * routing table.
 */
int rt_for_each(net_rt_t * rt, FRadixTreeForEach fForEach,
		void * ctx)
{
  _for_each_ctx_t for_each_ctx;

  for_each_ctx.fForEach= fForEach;
  for_each_ctx.ctx= ctx;

  return trie_for_each(rt, _rt_for_each_function, &for_each_ctx);
}


/////////////////////////////////////////////////////////////////////
//
// ROUTING TABLE DUMP FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- net_route_type_dump ----------------------------------------
/**
 *
 */
void net_route_type_dump(gds_stream_t * stream, net_route_type_t type)
{
  switch (type) {
  case NET_ROUTE_DIRECT:
    stream_printf(stream, "DIRECT");
    break;
  case NET_ROUTE_STATIC:
    stream_printf(stream, "STATIC");
    break;
  case NET_ROUTE_IGP:
    stream_printf(stream, "IGP");
    break;
  case NET_ROUTE_BGP:
    stream_printf(stream, "BGP");
    break;
  default:
    abort();
  }
}

// -----[ rt_info_dump ]---------------------------------------------
/**
 * Output format:
 * <dst-prefix> <link/if> <weight> <type> [ <state> ]
 */
void rt_info_dump(gds_stream_t * stream, const rt_info_t * rtinfo)
{
  const rt_entry_t * rtentry;
  unsigned int index, index2;
  int cr= 0;
  char pfx_str[19];

  for (index= 0; index < rt_entries_size(rtinfo->entries); index++) {
    rtentry= rt_entries_get_at(rtinfo->entries, index);
    if (!cr) {
      ip_prefix_dump(stream, rtinfo->prefix);
      ip_prefix_to_string(&rtinfo->prefix, pfx_str, sizeof(pfx_str));
      for (index2= 0; index2 < strlen(pfx_str); index2++)
	pfx_str[index2]= ' ';
    } else {
      stream_printf(stream, "\n%s", pfx_str);
    }
    cr= 1;
    stream_printf(stream, "\t");
    rt_entry_dump(stream, rtentry);
    stream_printf(stream, "\t%u\t", rtinfo->metric);
    net_route_type_dump(stream, rtinfo->type);
    if ((rtentry->oif != NULL) && !net_iface_is_enabled(rtentry->oif)) {
      stream_printf(stream, "\t[DOWN]");
    }
  }
}

// -----[ _rt_dump_for_each ]----------------------------------------
static int _rt_dump_for_each(uint32_t key, uint8_t key_len,
			     void * item, void * ctx)
{
  gds_stream_t * stream= (gds_stream_t *) ctx;
  rt_infos_t * list= (rt_infos_t *) item;
  ip_pfx_t pfx;
  
  pfx.network= key;
  pfx.mask= key_len;
  
  _rt_info_list_dump(stream, pfx, list);
  return 0;
}

// -----[ rt_dump ]--------------------------------------------------
/**
 * Dump the routing table for the given destination. The destination
 * can be of type NET_DEST_ANY, NET_DEST_ADDRESS and NET_DEST_PREFIX.
 */
void rt_dump(gds_stream_t * stream, net_rt_t * rt, ip_dest_t dest)
{
  rt_info_t * rtinfo;

  //fprintf(pStream, "DESTINATION\tNEXTHOP\tIFACE\tMETRIC\tTYPE\n");

  switch (dest.type) {

  case NET_DEST_ANY:
    trie_for_each(rt, _rt_dump_for_each, stream);
    break;

  case NET_DEST_ADDRESS:
    rtinfo= rt_find_best(rt, dest.prefix.network, NET_ROUTE_ANY);
    if (rtinfo != NULL) {
      rt_info_dump(stream, rtinfo);
      stream_printf(stream, "\n");
    }
    break;

  case NET_DEST_PREFIX:
    rtinfo= rt_find_exact(rt, dest.prefix, NET_ROUTE_ANY);
    if (rtinfo != NULL) {
      rt_info_dump(stream, rtinfo);
      stream_printf(stream, "\n");
    }
    break;

  default:
    abort();
  }
}

