// ==================================================================
// @(#)types.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/11/2005
// $Id: types.h,v 1.4 2009-03-24 15:55:15 bqu Exp $
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

/**
 * \file
 * Provide the main BGP data structures.
 */

#ifndef __BGP_TYPES_H__
#define __BGP_TYPES_H__

#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/radix-tree.h>
#include <libgds/trie.h>

#include <net/prefix.h>
#include <net/net_types.h>

#include <bgp/attr/types.h>
#include <bgp/filter/types.h>
#include <bgp/nlri.h>
#include <bgp/route_reflector.h>

//#define __ROUTER_LIST_ENABLE__
#define __BGP_ROUTE_INFO_DP__

// -----[ asn_t ]----------------------------------------------------
/** Maximum number of AS. */
#define MAX_AS 65536
/** Definition of an AS Number (ASN). */
typedef uint16_t asn_t;

// -----[ bgp_routes_t ]---------------------------------------------
/** Definition of a set of BGP routes. */
typedef ptr_array_t bgp_routes_t;


// -----[ bgp_peers_t ]----------------------------------------------
/** Definition of a set of BGP peers. */
typedef ptr_array_t bgp_peers_t;


// -----[ bgp_rib_dir_t ]--------------------------------------------
/** Adjacent RIB direction (input/output). */
typedef enum {
  RIB_IN,
  RIB_OUT,
  RIB_MAX
} bgp_rib_dir_t;


// -----[ bgp_rib_t ]------------------------------------------------
/** Definition of a BGP Routing Information Base (RIB). */
typedef gds_trie_t bgp_rib_t;


// -----[ BGP attribute reference counter ]--------------------------
typedef uint32_t bgp_attr_refcnt_t;


// -----[ bgp_domain_t ]---------------------------------------------
/** Definition of a BGP domain (AS). */
typedef struct bgp_domain_t {
  /** AS Number (ASN). */
  asn_t              asn;
  /** Name. */
  char             * name;
  /** Member routers. */
  gds_radix_tree_t * routers;
} bgp_domain_t;


// -----[ bgp_router_t ]---------------------------------------------
/** Definition of a BGP router. */
typedef struct bgp_router_t {
  /** AS Number (ASN). */
  asn_t                 asn;
  /** Router-ID (RID). */
  net_addr_t            rid;
  /** List of neighbor routers. */
  bgp_peers_t         * peers;
  /** Local Routing Information Base (Loc-RIB). */
  bgp_rib_t           * loc_rib;
  /** List of originated prefixes. */
  bgp_routes_t        * local_nets;
  /** Cluster-ID. */
  bgp_cluster_id_t      cluster_id;
  /** Flag: is route-reflector. */
  int                   reflector;
  /** Reference to parent node. */
  net_node_t          * node;
  /** Reference to BGP domain (AS). */
  struct bgp_domain_t * domain;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  /** This is a list of neighbors sorted on the walton limit number
   * \todo is it possible to get rid of the \c peers field when
   * using walton ? */
  ptr_array_t * pWaltonLimitPeers;
#endif
} bgp_router_t;


// -----[ bgp_peer_state_t ]-----------------------------------------
/** BGP session states. */
typedef enum {
  /** Idle state. */
  SESSION_STATE_IDLE,
  /** Openwait state. */
  SESSION_STATE_OPENWAIT,
  /** Established state. */
  SESSION_STATE_ESTABLISHED,
  /** Active state. */
  SESSION_STATE_ACTIVE,
  SESSION_STATE_MAX
} bgp_peer_state_t;


// -----[ bgp_peer_t ]-----------------------------------------------
/** Definition of a BGP neighbor. */
typedef struct bgp_peer_t {
  /** IP address used to reach the neighbor. */
  net_addr_t            addr;
  /** AS Number (ASN) of the neighbor. */
  uint16_t              asn;
  /** Reference to the local router (owner). */
  struct bgp_router_t * router;    
  /* Configuration flags. */
  uint8_t               flags;
  /* Router-ID of the neighbor.
   * The Router-ID is initialy 0. It is set when an OPEN message is
   * received. For virtual peers it is set to the peer's IP address
   * when the session is opened. */
  net_addr_t            router_id;
  /** Input and output filters. */
  struct bgp_filter_t * filter[FILTER_MAX];
  /** Input and output Adjacent Routing Information Bases (Adj-RIBs). */
  bgp_rib_t           * adj_rib[RIB_MAX];    
  /** Session state (handled by the FSM). */
  bgp_peer_state_t      session_state;
  /** Optional next-hop to advertise to this peer. */
  net_addr_t            next_hop;
  /** Optional source IP address. */
  net_addr_t            src_addr; 
  /** "TCP" send sequence number. */
  unsigned int          send_seq_num;
  /** "TCP" receive sequence number. */
  unsigned int          recv_seq_num;
  /** Last error of the session. */
  int                   last_error;
  /** Optionnal stream for recording sent/received BGP messages. */
  gds_stream_t        * pRecordStream;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uWaltonLimit;
#endif
} bgp_peer_t;


// -----[ BGP route attributes ]-------------------------------------
/** Definition of BGP route attributes. */
typedef struct bgp_attr_t {
  /** Next-hop. */
  net_addr_t           next_hop;
  /** Origin. */
  bgp_origin_t         origin;
  /** Local preference. */
  uint32_t             local_pref;
  /** Multi-Exit-Discriminator. */
  uint32_t             med;
  /** AS-Path. */
  bgp_path_t         * path_ref;
  /** Communities. */
  bgp_comms_t        * comms;
  /** Extended communities. */
  bgp_ecomms_t       * ecomms;

  /** Originator (Route reflection). */
  bgp_originator_t   * originator;
  /** Cluster-ID-List (Route reflection). */
  bgp_cluster_list_t * cluster_list;

#ifdef BGP_QOS
  /* QoS attributes (experimental) */
  qos_delay_t     delay;
  qos_bandwidth_t bandwidth;
  ptr_array_t     * pAggrRoutes;
  struct bgp_route_t * pAggrRoute;
  struct bgp_route_t * pEBGPRoute;
#endif

#ifdef __ROUTER_LIST_ENABLE__
  /** Router-List attribute (experimental) */
  bgp_cluster_list_t * router_list;
#endif
} bgp_attr_t;


// -----[ bgp_route_t ]----------------------------------------------
/** Definition of a BGP route. */
typedef struct bgp_route_t {
  /** Destination prefix. */
  ip_pfx_t            prefix;
  /** Neighbor who learned that route. */
  struct bgp_peer_t * peer;
  /** Flags (best, feasible, eligible, ...). */
  uint16_t            flags;
  /** Route attributes. */
  bgp_attr_t        * attr;

#ifdef __BGP_ROUTE_INFO_DP__
  /** How the route was selected (meaningful only if the route is
   *  currently best). */
  uint8_t             rank;   
#endif

#ifdef __EXPERIMENTAL__
  /** Origin router: used in combination with withdraw root cause.
   *  Identifies the BGP router that originated this route. */
  struct bgp_router_t * pOriginRouter;
#endif

  bgp_nlri_t          nlri;
} bgp_route_t;


// -----[ bgp_rcn_t ]------------------------------------------------
typedef uint8_t bgp_rcn_id_t;
typedef struct {
  bgp_rcn_id_t   id;
  bgp_router_t * router;
} bgp_rcn_t;


// -----[ bgp_msg_type_t ]-------------------------------------------
/** BGP message types. */
typedef enum {
  /** Update message. */
  BGP_MSG_TYPE_UPDATE,
  /** Withdraw message. */
  BGP_MSG_TYPE_WITHDRAW,
  /** Close message. */
  BGP_MSG_TYPE_CLOSE,
  /** Open message. */
  BGP_MSG_TYPE_OPEN,
  BGP_MSG_TYPE_MAX,
} bgp_msg_type_t;


// -----[ bgp_msg_t ]------------------------------------------------
/** Definition of a BGP message (abstract). */
typedef struct {
  /** Message type. */
  bgp_msg_type_t type;
  /** ASN of the source. */
  uint16_t       peer_asn;
  /** "TCP" sequence number (used for checking ordering). */
  unsigned int   seq_num;
} bgp_msg_t;


// -----[ bgp_msg_update_t ]-----------------------------------------
/** Definition of a BGP update message. */
typedef struct {
  /** Common BGP message header. */
  bgp_msg_t     header;
  /** BGP route payload. */
  bgp_route_t * route;
} bgp_msg_update_t;


// -----[ bgp_msg_withdraw_t ]---------------------------------------
/** Definition of a BGP withdraw message. */
typedef struct {
  /** Common BGP message header. */
  bgp_msg_t       header;
  /** Withdrawn prefix. */
  ip_pfx_t        prefix;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  /** Withdrawn next-hop. */
  net_addr_t    * next_hop;
#endif /* __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__ */
} bgp_msg_withdraw_t;


// -----[ bgp_msg_close_t ]------------------------------------------
/** Definition of a BGP close message. */
typedef struct {
  /** Common BGP message header. */
  bgp_msg_t header;
} bgp_msg_close_t;


// -----[ bgp_msg_open_t ]-------------------------------------------
/** Definition of a BGP open message. */
typedef struct {
  /** Common BGP message header. */
  bgp_msg_t  header;
  /** Router-ID of the source. */
  net_addr_t router_id;
} bgp_msg_open_t;


#endif /* __BGP_TYPES_H__ */
