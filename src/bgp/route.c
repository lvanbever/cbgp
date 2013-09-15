// ==================================================================
// @(#)route.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/11/2002
// $Id: route.c,v 1.31 2009-03-24 15:51:30 bqu Exp $
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

#include <libgds/array.h>
#include <libgds/memory.h>
#include <libgds/str_util.h>

#include <bgp/attr.h>
#include <bgp/attr/comm.h>
#include <bgp/attr/ecomm.h>
#include <bgp/attr/origin.h>
#include <bgp/qos.h>
#include <bgp/route.h>
#include <util/str_format.h>

typedef struct _options_t {
  uint8_t   show_mode;
  char    * show_format;
} _options_t;
static _options_t _default_options= {
  .show_mode  = BGP_ROUTES_OUTPUT_CISCO,
  .show_format= NULL,
};

// -----[ Forward prototypes declaration ]---------------------------
/* Note: functions starting with underscore (_) are intended to be
 * used inside this file only (private). These functions should be
 * static and should not appear in the .h file.
 */
static inline bgp_route_t *
_route_create2(ip_pfx_t prefix, bgp_peer_t * peer, bgp_attr_t * attr);

// ----- route_create -----------------------------------------------
/**
 * Create a new BGP route towards the given prefix. The 'peer' field
 * mentions the peer who announced the route to the local router. This
 * field can be NULL if the route was originated locally or if the
 * peer is unknown. The 'next-hop' field is the BGP next-hop of the
 * route. Finally, the 'origin-type' field tells how the route was
 * originated. This is usually set to IGP.
 */
bgp_route_t * route_create(ip_pfx_t prefix, bgp_peer_t * peer,
			   net_addr_t next_hop, bgp_origin_t origin)
{
  return _route_create2(prefix, peer,
			bgp_attr_create(next_hop, origin,
					ROUTE_PREF_DEFAULT,
					ROUTE_MED_DEFAULT));
}

// -----[ _route_create2 ]-------------------------------------------
/**
 *
 */
static inline bgp_route_t *
_route_create2(ip_pfx_t prefix, bgp_peer_t * peer, bgp_attr_t * attr)
{
  bgp_route_t * route= (bgp_route_t *) MALLOC(sizeof(bgp_route_t));
  route->prefix= prefix;
  route->peer= peer;
  route->attr= attr;
  route->flags= 0;

#ifdef BGP_QOS
  bgp_route_qos_init(route);
#endif

#ifdef __EXPERIMENTAL__
  route->pOriginRouter= NULL;
#endif

#ifdef __BGP_ROUTE_INFO_DP__
  route->rank= 0;
#endif

  return route;
}

// -----[ route_create_nlri ]--------------------------------------
bgp_route_t * route_create_nlri(bgp_nlri_t nlri, bgp_peer_t * peer,
				net_addr_t next_hop, bgp_origin_t origin,
				bgp_attr_t * attr)
{
  bgp_route_t * route;
  size_t nlri_size= bgp_nlri_size(&nlri);
  size_t route_size= sizeof(bgp_route_t) + nlri_size - sizeof(bgp_nlri_t);

  route= (bgp_route_t *) MALLOC(route_size);
  route->peer= peer;
  //route->attr= attr;
  route->flags= 0;

#ifdef BGP_QOS
  bgp_route_qos_init(route);
#endif

#ifdef __EXPERIMENTAL__
  route->pOriginRouter= NULL;
#endif

#ifdef __BGP_ROUTE_INFO_DP__
  route->rank= 0;
#endif

  memcpy(&route->nlri, &nlri, nlri_size);

  return route;
}


// ----- route_destroy ----------------------------------------------
/**
 * Destroy the given route.
 */
void route_destroy(bgp_route_t ** route_ref)
{
  if (*route_ref != NULL) {

#ifdef BGP_QOS
    bgp_route_qos_destroy(*route_ref);
#endif

    bgp_attr_destroy(&(*route_ref)->attr);

    FREE(*route_ref);
    *route_ref= NULL;
  }
}

// ----- route_flag_set ---------------------------------------------
/**
 *
 */
inline void route_flag_set(bgp_route_t * route, uint16_t flag,
			   int state)
{
  if (state)
    route->flags|= flag;
  else
    route->flags&= ~flag;
}

// ----- route_flag_get ---------------------------------------------
/**
 *
 */
inline int route_flag_get(bgp_route_t * route, uint16_t flag)
{
  return route->flags & flag;
}

// ----- route_peer_set ---------------------------------------------
/**
 *
 */
inline void route_peer_set(bgp_route_t * route, bgp_peer_t * peer)
{
  route->peer= peer;
}

// ----- route_peer_get ---------------------------------------------
/**
 *
 */
inline bgp_peer_t * route_peer_get(bgp_route_t * route)
{
  assert(route->peer != NULL);
  return route->peer;
}

#ifdef __EXPERIMENTAL__
// ----- route_set_origin_router ------------------------------------
inline void route_set_origin_router(bgp_route_t * route,
				    bgp_router_t * router)
{
  route->pOriginRouter= router;
}
#endif

/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE NEXT-HOP
//
/////////////////////////////////////////////////////////////////////

// ----- route_set_nexthop ------------------------------------------
/**
 * Set the route's BGP next-hop.
 */
inline void route_set_nexthop(bgp_route_t * route,
			      net_addr_t next_hop)
{
  bgp_attr_set_nexthop(&route->attr, next_hop);
}

// ----- route_get_nexthop ------------------------------------------
/**
 * Return the route's BGP next-hop.
 */
inline net_addr_t route_get_nexthop(bgp_route_t * route)
{
  return route->attr->next_hop;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE ORIGIN
//
/////////////////////////////////////////////////////////////////////

// ----- route_set_origin -------------------------------------------
/**
 * Set the route's origin.
 */
inline void route_set_origin(bgp_route_t * route, bgp_origin_t origin)
{
  bgp_attr_set_origin(&route->attr, origin);
}

// ----- route_get_origin -------------------------------------------
/**
 * Return the route's origin.
 */
inline bgp_origin_t route_get_origin(bgp_route_t * route)
{
  return route->attr->origin;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE AS-PATH
//
/////////////////////////////////////////////////////////////////////

// -----[ route_set_path ]-------------------------------------------
/**
 * Set the route's AS-Path and adds a global reference if required.
 *
 * Note: the given AS-Path is not copied, but it will be referenced
 * in the global path repository if used or destroyed if not.
 */
inline void route_set_path(bgp_route_t * route, bgp_path_t * path)
{
  bgp_attr_set_path(&route->attr, path);
}

// -----[ route_get_path ]-------------------------------------------
/**
 * Return the route's AS-Path.
 */
inline bgp_path_t * route_get_path(bgp_route_t * route)
{
  return route->attr->path_ref;
}

// -----[ route_path_prepend ]---------------------------------------
/**
 * Prepend multiple times an AS-Number at the beginning of this
 * AS-Path. This is used when a route is propagated over an eBGP
 * session, for instance.
 *
 * The function first creates a copy of the current path. Then, it
 * removes the current AS-Path from the global path
 * repository. Finally, it appends the given AS-number to the AS-Path
 * copy. If the prepending could be done without error, the resulting
 * path is set as the route's AS-Path.
 *
 * Return value:
 *   0  in case of success
 *  -1  in case of failure (AS-Path would become too long)
 */
inline int route_path_prepend(bgp_route_t * route, asn_t asn,
			      uint8_t amount)
{
  return bgp_attr_path_prepend(&route->attr, asn, amount);
}

// -----[ route_path_rem_private ]-----------------------------------
inline int route_path_rem_private(bgp_route_t * route)
{
  return bgp_attr_path_rem_private(&route->attr);
}

// ----- route_path_contains ----------------------------------------
/**
 * Test if the route's AS-PAth contains the given AS-number.
 *
 * Returns 1 if true, 0 otherwise.
 */
inline int route_path_contains(bgp_route_t * route, asn_t asn)
{
  return (route->attr->path_ref != NULL) &&
    (path_contains(route->attr->path_ref, asn) != 0);
}

// ----- route_path_length ------------------------------------------
/**
 * Return the length of the route's AS-Path.
 */
inline int route_path_length(bgp_route_t * route)
{
  if (route->attr->path_ref != NULL)
    return path_length(route->attr->path_ref);
  else
    return 0;
}

// ----- route_path_last_as -----------------------------------------
/**
 * Return the last AS-number in the AS-Path. This function is
 * currently used in dp_rules.c in order to implement the MED-based
 * rule.
 *
 * Note: this function is EXPERIMENTAL and should be updated. It does
 * not work if the last segment in the AS-Path is of type AS-SET since
 * there is no ordering of the AS-numbers in this case.
 */
inline int route_path_last_as(bgp_route_t * route)
{
  if (route->attr->path_ref != NULL)
    return path_last_as(route->attr->path_ref);
  else
    return -1;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE COMMUNITIES
//
/////////////////////////////////////////////////////////////////////

// ----- route_set_comm ---------------------------------------------
/**
 * Set the route's Communities attribute and adds a global reference
 * if required.
 *
 * Note: the given Communities is not copied, but it will be
 * referenced in the global Communities repository if used or
 * destroyed if not.
 */
inline void route_set_comm(bgp_route_t * route, bgp_comms_t * comms)
{
  bgp_attr_set_comm(&route->attr, comms);
}

// ----- route_comm_append ------------------------------------------
/**
 * Add a new community value to the Communities attribute.
 *
 * Return value:
 *    0  in case of success
 *   -1  in case of failure (Communities would become too long)
 */
inline int route_comm_append(bgp_route_t * route, bgp_comm_t comm)
{
  return bgp_attr_comm_append(&route->attr, comm);
}

// ----- route_comm_strip -------------------------------------------
/**
 * Remove all the community values from the Communities attribute.
 */
inline void route_comm_strip(bgp_route_t * route)
{
  bgp_attr_comm_strip(&route->attr);
}

// ----- route_comm_remove ------------------------------------------
/**
 * Remove a single community value from the Communities attribute.
 */
inline void route_comm_remove(bgp_route_t * route, bgp_comm_t comm)
{
  bgp_attr_comm_remove(&route->attr, comm);
}

// ----- route_comm_contains ----------------------------------------
/**
 * Test if the Communities attribute contains the given community
 * value.
 */
inline int route_comm_contains(bgp_route_t * route, bgp_comm_t comm)
{
  return (route->attr->comms != NULL) &&
    (comms_contains(route->attr->comms, comm) != 0);
}

/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE EXTENDED-COMMUNITIES
//
/////////////////////////////////////////////////////////////////////

// ----- route_ecomm_append -----------------------------------------
/**
 *
 */
inline int route_ecomm_append(bgp_route_t * route, bgp_ecomm_t * comm)
{
  return bgp_attr_ecomm_append(&route->attr, comm);
}

// ----- route_ecomm_strip_non_transitive ---------------------------
/**
 *
 */
inline void route_ecomm_strip_non_transitive(bgp_route_t * route)
{
  ecomms_strip_non_transitive(&route->attr->ecomms);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE LOCAL-PREF
//
/////////////////////////////////////////////////////////////////////

// ----- route_localpref_set ----------------------------------------
/**
 *
 */
inline void route_localpref_set(bgp_route_t * route, uint32_t pref)
{
  route->attr->local_pref= pref;
}

// ----- route_localpref_get ----------------------------------------
/**
 *
 */
inline uint32_t route_localpref_get(bgp_route_t * route)
{
  return route->attr->local_pref;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE MULTI-EXIT-DISCRIMINATOR (MED)
//
/////////////////////////////////////////////////////////////////////

// ----- route_med_clear --------------------------------------------
/**
 *
 */
inline void route_med_clear(bgp_route_t * route)
{
  route->attr->med= ROUTE_MED_MISSING;
}

// ----- route_med_set ----------------------------------------------
/**
 *
 */
inline void route_med_set(bgp_route_t * route, uint32_t med)
{
  route->attr->med= med;
}

// ----- route_med_get ----------------------------------------------
/**
 *
 */
inline uint32_t route_med_get(bgp_route_t * route)
{
  return route->attr->med;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE ORIGINATOR
//
/////////////////////////////////////////////////////////////////////

// ----- route_originator_set ---------------------------------------
/**
 *
 */
inline void route_originator_set(bgp_route_t * route, net_addr_t originator)
{
  assert(route->attr->originator == NULL);
  route->attr->originator= (net_addr_t *) MALLOC(sizeof(net_addr_t));
  memcpy(route->attr->originator, &originator, sizeof(net_addr_t));
}

// ----- route_originator_get ---------------------------------------
/**
 * Return the route's Originator-ID (if set).
 *
 * Return values:
 *    0 if Originator-ID exists
 *   -1 otherwise
 */
inline int route_originator_get(bgp_route_t * route, net_addr_t * originator)
{
  if (route->attr->originator != NULL) {
    if (originator != NULL)
      memcpy(originator, route->attr->originator, sizeof(net_addr_t));
    return 0;
  } else
    return -1;
}

// ----- route_originator_clear -------------------------------------
/**
 *
 */
inline void route_originator_clear(bgp_route_t * route)
{
  bgp_attr_originator_destroy(route->attr);
}

// ----- route_originator_equals ------------------------------------
/**
 * Compare the Originator-ID of two routes.
 *
 * Return value:
 *   1  if both originator-IDs are equal
 *   0  otherwise
 */
inline int route_originator_equals(bgp_route_t * route1, bgp_route_t * route2)
{
  return originator_equals(route1->attr->originator,
			   route2->attr->originator);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE CLUSTER-ID-LIST
//
/////////////////////////////////////////////////////////////////////

// ----- route_cluster_list_set -------------------------------------
/**
 *
 */
inline void route_cluster_list_set(bgp_route_t * route)
{
  assert(route->attr->cluster_list == NULL);
  route->attr->cluster_list= cluster_list_create();
}

// ----- route_cluster_list_append ----------------------------------
/**
 *
 */
inline void route_cluster_list_append(bgp_route_t * route,
				      bgp_cluster_id_t cluster_id)
{
  if (route->attr->cluster_list == NULL)
    route_cluster_list_set(route);
  cluster_list_append(route->attr->cluster_list, cluster_id);
}

// ----- route_cluster_list_clear -----------------------------------
/**
 *
 */
void route_cluster_list_clear(bgp_route_t * route)
{
  bgp_attr_cluster_list_destroy(route->attr);
}

// ----- route_cluster_list_equals ----------------------------------
/**
 * Compare the Cluster-ID-List of two routes.
 *
 * Return value:
 *   1  if both Cluster-ID-Lists are equal
 *   0  otherwise
 */
inline int route_cluster_list_equals(bgp_route_t * route1, bgp_route_t * route2)
{
  return cluster_list_equals(route1->attr->cluster_list,
			     route2->attr->cluster_list);
}

// ----- route_cluster_list_contains --------------------------------
/**
 *
 */
inline int route_cluster_list_contains(bgp_route_t * route,
				       bgp_cluster_id_t cluster_id)
{
  if (route->attr->cluster_list != NULL)
    return cluster_list_contains(route->attr->cluster_list, cluster_id);
  return 0;
}


/////////////////////////////////////////////////////////////////////
//
// ROUTE COPY & COMPARE
//
/////////////////////////////////////////////////////////////////////

// ----- route_equals -----------------------------------------------
/**
 *
 */
int route_equals(bgp_route_t * route1, bgp_route_t * route2)
{
  if (route1 == route2)
    return 1;

#ifdef BGP_QOS
  if (!bgp_route_qos_equals(route1, route2))
    return 0;
#endif

  // Compare destination prefixes
  if (ip_prefix_cmp(&route1->prefix, &route2->prefix))
    return 0;

  if (bgp_attr_cmp(route1->attr, route2->attr))
    return 1;

  return 0;
}

// ----- route_copy -------------------------------------------------
/**
 *
 */
bgp_route_t * route_copy(bgp_route_t * route)
{
  bgp_route_t * new_route= _route_create2(route->prefix,
					  route->peer,
					  bgp_attr_copy(route->attr));

  /* Route info */
  new_route->flags= route->flags;

#ifdef BGP_QOS
  bgp_route_qos_copy(new_route, route);
#endif

#ifdef __BGP_ROUTE_DP_INFO__
  new_route->rank= route->rank;
#endif

  return new_route;
}


/////////////////////////////////////////////////////////////////////
//
// ROUTE DUMP FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ route_str2mode ]-------------------------------------------
int route_str2mode(const char * str, uint8_t * mode)
{
  if (!strcmp(str, "cisco") || !strcmp(str, "default")) {
    *mode= BGP_ROUTES_OUTPUT_CISCO;
    return 0;
  } else if (!strcmp(str, "mrt") || !strcmp(str, "mrt-ascii")) {
    *mode= BGP_ROUTES_OUTPUT_MRT_ASCII;
    return 0;
  } else if (!strcmp(str, "custom")) {
    *mode= BGP_ROUTES_OUTPUT_CUSTOM;
    return 0;
  }
  return -1;
}

// ----- route_dump -------------------------------------------------
/**
 * Dump a BGP route.
 */
void route_dump(gds_stream_t * stream, bgp_route_t * route)
{
  switch (_default_options.show_mode) {
  case BGP_ROUTES_OUTPUT_MRT_ASCII:
    route_dump_mrt(stream, route);
    break;
  case BGP_ROUTES_OUTPUT_CUSTOM:
    route_dump_custom(stream, route, _default_options.show_format);
    break;
  case BGP_ROUTES_OUTPUT_CISCO:
  default:
    route_dump_cisco(stream, route);
  }
}

// ----- route_dump_cisco -------------------------------------------
/**
 * CISCO-like dump of routes.
 */
void route_dump_cisco(gds_stream_t * stream, bgp_route_t * route)
{
  if (route != NULL) {

    // Status code:
    // '*' - valid (feasible)
    // 'i' - internal (learned through 'add network')
    if (route_flag_get(route, ROUTE_FLAG_INTERNAL))
      stream_printf(stream, "i");
    else if (route_flag_get(route, ROUTE_FLAG_FEASIBLE))
      stream_printf(stream, "*");
    else
      stream_printf(stream, " ");

    // Best ?
    if (route_flag_get(route, ROUTE_FLAG_BEST))
      stream_printf(stream, ">");
    else
      stream_printf(stream, " ");

    stream_printf(stream, " ");

    // Prefix
    ip_prefix_dump(stream, route->prefix);

    // Next-Hop
    stream_printf(stream, "\t");
    ip_address_dump(stream, route->attr->next_hop);

    // Local-Pref & Multi-Exit-Distriminator
    stream_printf(stream, "\t%u\t%u\t", route->attr->local_pref,
	       route->attr->med);

    // AS-Path
    if (route->attr->path_ref != NULL)
      path_dump(stream, route->attr->path_ref, 1);
    else
      stream_printf(stream, "null");

    // Origin
    stream_printf(stream, "\t");
    switch (route->attr->origin) {
    case BGP_ORIGIN_IGP: stream_printf(stream, "i"); break;
    case BGP_ORIGIN_EGP: stream_printf(stream, "e"); break;
    case BGP_ORIGIN_INCOMPLETE: stream_printf(stream, "?"); break;
    default:
      cbgp_fatal("invalid BGP origin attribute (%d)", route->attr->origin);
    }

  } else
    stream_printf(stream, "(null)");

#ifdef __ROUTER_LIST_ENABLE__
  if (route->attr->router_list != NULL) {
    stream_printf(stream, "[router-list=");
    cluster_list_dump(stream, route->attr->router_list);
    stream_printf(stream, "\n");
  }
#endif
}

// ----- route_dump_mrt --------------------------------------------
/**
 * Dump a route in MRTD format. The output has thus the following
 * format:
 *
 *   <best>|<peer-IP>|<peer-AS>|<prefix>|<AS-path>|<origin>|
 *   <next-hop>|<local-pref>|<med>|<comm>|<ext-comm>|
 *
 * where <best> is "B" if the route is marked as best and "?"
 * otherwise. Note that the <ext-comm> and <delay> fields are not
 * standard MRTD fields.
 *
 * Additional fields are dumped which are not standard:
 *
 *   <delay>|<delay-mean>|<delay-weight>|
 *   <bandwidth>|<bandwidth-mean>|<bandwidth-weight>
 *
 */
void route_dump_mrt(gds_stream_t * stream, bgp_route_t * route)
{
  if (route == NULL) {
    stream_printf(stream, "null");
  } else {

    // BEST flag: in MRT's route_btoa, table dumps only contain best
    // routes, so that a "B" is written. In our case, when dumping an
    // Adj-RIB-In, non-best routes might exist. For these routes, a
    // "_" is written.
    if (route_flag_get(route, ROUTE_FLAG_BEST)) {
      stream_printf(stream, "B|");
    } else {
      stream_printf(stream, "_|");
    }

    // Peer-IP: if the route is local, the keyword LOCAL is written in
    // place of the peer's IP address and AS number.
    if (route->peer != NULL) {
      ip_address_dump(stream, route->peer->addr);
      // Peer-AS
      stream_printf(stream, "|%u|", route->peer->asn);
    } else {
      stream_printf(stream, "LOCAL|LOCAL|");
    }

    // Prefix
    ip_prefix_dump(stream, route->prefix);
    stream_printf(stream, "|");

    // AS-Path
    path_dump(stream, route->attr->path_ref, 1);
    stream_printf(stream, "|");

    // Origin
    stream_printf(stream, bgp_origin_to_str(route->attr->origin));
    stream_printf(stream, "|");

    // Next-Hop
    ip_address_dump(stream, route->attr->next_hop);

    // Local-Pref
    stream_printf(stream, "|%u|", route->attr->local_pref);

    // Multi-Exit-Discriminator
    if (route->attr->med != ROUTE_MED_MISSING)
      stream_printf(stream, "%u|", route->attr->med);
    else
      stream_printf(stream, "|");

    // Communities: written as numeric values
    if (route->attr->comms != NULL)
      comm_dump(stream, route->attr->comms, COMM_DUMP_RAW);
    stream_printf(stream, "|");

    // Extended-Communities: written as numeric values
    if (route->attr->ecomms != NULL)
      ecomm_dump(stream, route->attr->ecomms, ECOMM_DUMP_RAW);
    stream_printf(stream, "|");

    // Route-reflectors: Originator-ID
    if (route->attr->originator != NULL)
      ip_address_dump(stream, *route->attr->originator);
    stream_printf(stream, "|");

    // Route-reflectors: Cluster-ID-List
    if (route->attr->cluster_list != NULL)
      cluster_list_dump(stream, route->attr->cluster_list);
    stream_printf(stream, "|");
    
    /*
    // QoS Delay
    fprintf(stream, "%u|%u|%u|", route->tDelay.tDelay,
	    route->tDelay.tMean, route->tDelay.uWeight);
    // QoS Bandwidth
    fprintf(stream, "%u|%u|%u|", route->tBandwidth.uBandwidth,
	    route->tBandwidth.uMean, route->tBandwidth.uWeight);
    */

  }
}

// ----- route_dump_custom ------------------------------------------
/**
 * Dump a BGP route to the given stream. The route fields are written
 * to the stream according to a customizable format specifier string.
 *
 * Specification  | Field
 * ---------------+------------------------
 * %i             | Peer-IP
 * %a             | Peer-AS
 * %p             | Prefix
 * %l             | Local-Preference
 * %P             | AS-Path
 * %m             | MED
 * %o             | Origin
 * %n             | Next-Hop
 * %c             | Communities
 * %e             | Extended-Communities
 * %O             | Originator-ID
 * %C             | Cluster-ID-List
 * %A             | origin AS
 */
static int _route_dump_custom(gds_stream_t * stream, void * pContext,
			      char cFormat)
{
  bgp_route_t * route= (bgp_route_t *) pContext;
  int asn;
  switch (cFormat) {
  case 'i':
    if (route->peer != NULL)
      ip_address_dump(stream, route->peer->addr);
    else
      stream_printf(stream, "LOCAL");
    break;
  case 'a':
    if (route->peer != NULL)
      stream_printf(stream, "%u", route->peer->asn);
    else
      stream_printf(stream, "LOCAL");
    break;
  case 'p':
    ip_prefix_dump(stream, route->prefix);
    break;
  case 'l':
    stream_printf(stream, "%u", route->attr->local_pref);
    break;
  case 'P':
    path_dump(stream, route->attr->path_ref, 1);
    break;
  case 'o':
    stream_printf(stream, bgp_origin_to_str(route->attr->origin));
    break;
  case 'm':
    stream_printf(stream, "%u", route->attr->med);
    break;
  case 'n':
    ip_address_dump(stream, route->attr->next_hop);
    break;
  case 'c':
    if (route->attr->comms != NULL)
      comm_dump(stream, route->attr->comms, COMM_DUMP_RAW);
    break;
  case 'e':
    if (route->attr->ecomms != NULL)
      ecomm_dump(stream, route->attr->ecomms, ECOMM_DUMP_RAW);
    break;
  case 'O':
    if (route->attr->originator != NULL)
      ip_address_dump(stream, *route->attr->originator);
    break;
  case 'C':
    if (route->attr->cluster_list != NULL)
      cluster_list_dump(stream, route->attr->cluster_list);
    break;
  case 'A':
    asn= path_first_as(route->attr->path_ref);
    if (asn >= 0) {
      stream_printf(stream, "%u", asn);
    } else {
      stream_printf(stream, "*");
    }
    break;
  default:
    return -1;
  }
  return 0;
}

void route_dump_custom(gds_stream_t * stream, bgp_route_t * route,
		       const char * format_str)
{
  int result= 
    str_format_for_each(stream, _route_dump_custom, route, format_str);
  assert(result == 0);
}

// -----[ route_set_show_mode ]--------------------------------------
void route_set_show_mode(uint8_t mode, const char * format)
{
  _default_options.show_mode= mode;
  str_destroy(&_default_options.show_format);
  if (format != NULL)
    _default_options.show_format= str_create(format);
}


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION & FINALIZATION
// 
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_route_destroy ]--------------------------------------
void _bgp_route_destroy()
{
  str_destroy(&_default_options.show_format);
}
