// ==================================================================
// @(#)net_types.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 30/06/2005
// $Id: net_types.h,v 1.18 2009-03-24 16:18:43 bqu Exp $
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
 * Provide the main network data structures.
 */

#ifndef __NET_TYPES_H__
#define __NET_TYPES_H__

#include <libgds/array.h>
#include <libgds/radix-tree.h>

#include <net/link_attr.h>
#include <net/prefix.h>
#include <net/routing_t.h>
#include <sim/simulator.h>

typedef uint8_t net_tos_t;
typedef uint8_t   ospf_dest_type_t;
typedef uint32_t  ospf_area_t;
typedef uint8_t   ospf_path_type_t;
typedef ptr_array_t next_hops_list_t;
typedef ptr_array_t SOSPFRouteInfoList;

typedef ptr_array_t net_ifaces_t;
typedef ptr_array_t net_subnets_t;
typedef ptr_array_t igp_domains_t;

struct ip_opt_t;

typedef uint32_t net_link_delay_t;
#define NET_LINK_DELAY_INFINITE UINT32_MAX
typedef uint32_t net_link_load_t;
#define NET_LINK_MAX_CAPACITY UINT32_MAX
#define NET_LINK_MAX_LOAD UINT32_MAX


// -----[ Forward declarations ]-------------------------------------
struct spt_t;

// -----[ coord_t ]--------------------------------------------------
/** Definition of geographical coordinates. */
typedef struct {
  float latitude;    
  float longitude;
} coord_t;


//////////////////////////////////////////////////////////////////////
//
// MESSAGE STRUCTURE
//
//////////////////////////////////////////////////////////////////////

// -----[ net_protocol_id_t ]----------------------------------------
/** Supported network protocols. */
typedef enum {
  /** Raw data protocol. */
  NET_PROTOCOL_RAW,
  /** Protocol used for debug purposes. */
  NET_PROTOCOL_DEBUG,
  /** ICMP protocol. */
  NET_PROTOCOL_ICMP,
  /** BGP protocol. */
  NET_PROTOCOL_BGP,
  /** IP-in-IP tunneling protocol. */
  NET_PROTOCOL_IPIP,
  NET_PROTOCOL_MAX,
} net_protocol_id_t;

// ----- FPayLoadDestroy -----
typedef void (*FPayLoadDestroy)(void ** payload);


// -----[ net_msg_ops_t ]--------------------------------------------
/** Virtual methods of a network message. */
typedef struct {
  FPayLoadDestroy destroy;
} net_msg_ops_t;


// -----[ net_msg_t ]------------------------------------------------
/** Definition of a network message. */
typedef struct {
  /** Source IP address. */
  net_addr_t          src_addr;
  /** Destination IP address. */
  net_addr_t          dst_addr;
  /** Protocol identifier. */
  net_protocol_id_t   protocol;
  /** Time-to-live. */
  uint8_t             ttl;
  /** Type of service. */
  net_tos_t           tos;
  /** Payload. */
  void              * payload;
  /** Virtual methods. */
  net_msg_ops_t       ops;
  /** Options. */
  struct ip_opt_t   * opts;
} net_msg_t;


// -----[ net_protocol_ops_t ]---------------------------------------
/** Virtual methods of a network protocol. */
typedef struct {
  int    (*handle)      (simulator_t * sim, void * handler,
			 net_msg_t * msg);
  void   (*destroy)     (void ** handler);
  void   (*dump_msg)    (gds_stream_t * stream, net_msg_t * msg);
  void   (*destroy_msg) (net_msg_t * msg);
  void * (*copy_payload)(net_msg_t * msg);
} net_protocol_ops_t;


// -----[ net_protocol_def_t ]---------------------------------------
/** Definition of a network protocol. */
typedef struct {
  const char         * name;  
  net_protocol_ops_t   ops;
} net_protocol_def_t;


// -----[ net_protocol_t ]-------------------------------------------
/** Definition of a network protocol handler. */
typedef struct {
  net_protocol_id_t          id;
  void                     * handler;
  const net_protocol_def_t * def;
} net_protocol_t;


// -----[ net_protocols_t ]------------------------------------------
/** Array of network protocol handlers. */
typedef ptr_array_t net_protocols_t;


/////////////////////////////////////////////////////////////////////
//
// MAIN NETWORK STRUCTURE
//
/////////////////////////////////////////////////////////////////////

// -----[ network_t ]------------------------------------------------
/** Definition of a network database. */
typedef struct {
  /** List of IGP domains. */
  igp_domains_t * domains;
  /** Set of nodes (key=IP address). */
  gds_trie_t    * nodes;
  /** Set of subnets (key=IP prefix) */
  net_subnets_t * subnets;
  /** Network simulator. */
  simulator_t   * sim;
} network_t;


/////////////////////////////////////////////////////////////////////
//
// IGP DOMAIN TYPE DEFINITION
//
/////////////////////////////////////////////////////////////////////

// -----[ igp_domain_type_t ]----------------------------------------
/** Supported IGP domain types. */
typedef enum {
  /** Basic IGP domain (SPT, ECMP). */
  IGP_DOMAIN_IGP,
  /** OSPF model (SPT, ECMP, Areas, ...). */
  IGP_DOMAIN_OSPF,
  IGP_DOMAIN_MAX
} igp_domain_type_t;


// -----[ igp_domain_t ]---------------------------------------------
/** Definition of an IGP domain. */
typedef struct {
  /** Identifier. */
  uint16_t            id;
  /** Name of the domain. */
  char              * name;
  /** Set of member routers. */
  gds_trie_t        * routers;
  /** IGP domain type. */
  igp_domain_type_t   type;
} igp_domain_t;


//////////////////////////////////////////////////////////////////////
//
// OSPF TYPES DEFINITION
//
//////////////////////////////////////////////////////////////////////

#define ospf_ri_route_type_is(N, T) ((N)->tType == T)
#define ospf_ri_dest_type_is(N, T) ((N)->tOSPFDestinationType == T)
#define ospf_ri_area_is(N, A) ((N)->tOSPFArea == A)
#define ospf_ri_pathType_is(N, T) ((N)->tOSPFPathType == T)

#define ospf_ri_get_area(R) ((R)->tOSPFArea)
#define ospf_ri_get_prefix(R) ((R)->sPrefix)
#define ospf_ri_get_cost(R) ((R)->uWeight)
#define ospf_ri_get_pathType(R) ((R)->tOSPFPathType)
#define ospf_ri_get_NextHops(R) ((R)->aNextHops)

typedef struct {
  ospf_dest_type_t   tOSPFDestinationType;
  ip_pfx_t           sPrefix;
  net_link_delay_t   uWeight;
  ospf_area_t        tOSPFArea;
  ospf_path_type_t   tOSPFPathType;
  next_hops_list_t * aNextHops;
  net_route_type_t   tType;
} SOSPFRouteInfo;

typedef gds_trie_t SOSPFRT;


//////////////////////////////////////////////////////////////////////
//
// NODE TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// ----- net_node_t ---------------------------------------------------
/**
 * Definition of a network node data structure.
 *
 * This data structure represents any node in a CBGP simulation. The
 * data structure contains a node identifier (the RID) which is
 * unique. The RID currently takes the form of an IP address. The data
 * structures also holds the list of network interfaces, the
 * routing/forwarding table, the list of IGP domains the node belongs
 * to and the list of supported protocols.
 */
typedef struct {
  /** Identifier of the node (currently an IP address). */
  net_addr_t        rid;
  /** Name of the node. */
  char            * name;
  /** Attached network database. */
  network_t       * network;
  /** List of network interfaces. */
  net_ifaces_t    * ifaces;
  /** Routing/forwarding table. */
  net_rt_t        * rt;
    /** List of IGP domains. */
  uint16_array_t  * domains;
  /** Supported protocols. */
  net_protocols_t * protocols;
  /** Main IGP domain. */
  igp_domain_t    * domain;
  /** Geographical coordinates. */
  coord_t           coord;
  /** Individual syslog. */
  int               syslog_enabled;

#ifdef OSPF_SUPPORT
  /** List of OSPF areas. */
  uint32_array_t  * pOSPFAreas; 
  /** OSPF routing table. */
  SOSPFRT         * pOspfRT;
  /** OSPF ID. */
//  net_addr_t      ospf_id;
#endif

  struct spt_t    * spt;
} net_node_t;


//////////////////////////////////////////////////////////////////////
//
// SUBNET TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// -----[ net_subnet_type_t ]----------------------------------------
/** Types of network subnets. */
typedef enum {
  /** A transit subnet. */
  NET_SUBNET_TYPE_TRANSIT,
  /** A stub subnet. */
  NET_SUBNET_TYPE_STUB,
  NET_SUBNET_TYPE_MAX
} net_subnet_type_t;

// -----[ net_subnet_t ]---------------------------------------------
/**
 * Definition of a network subnet data structure.
 *
 * The subnet data structure is used to represent transit and stub
 * subnets. Transit subnets are announced in the IGP while stub
 * subnets are not. The data structure holds the subnet identifier
 * which takes the form of an IP prefix (IP address + mask length).
 * The data structure also maintains the list of network interfaces
 * connected to the subnet.
 */
typedef struct {
  /** Subnet identifier. */
  ip_pfx_t            prefix;
  /** Subnet type (transit/stub). */
  net_subnet_type_t   type;
  /** Attached interfaces. */
  net_ifaces_t      * ifaces;

#ifdef OSPF_SUPPORT
  /** Attached OSPF area. */
  ospf_area_t         uOSPFArea;
#endif
} net_subnet_t;


//////////////////////////////////////////////////////////////////////
//
// LINK TYPE DEFINITION
//
//////////////////////////////////////////////////////////////////////

// -----[ net_iface_type_t ]-----------------------------------------
/** Types of network interfaces. */
typedef enum {
  /** Loopback interface. */
  NET_IFACE_LOOPBACK,
  /** Un-assigned PTP interface. */
  NET_IFACE_RTR,
  /** Point-to-point interface. */
  NET_IFACE_PTP,
  /** Point-to-multipoint interface. */
  NET_IFACE_PTMP,
  /** Virtual interface (currently used for tunneling only). */
  NET_IFACE_VIRTUAL,
  NET_IFACE_MAX,
} net_iface_type_t;

struct net_iface_t;

// -----[ net_iface_send_f ]-----------------------------------------
/**
 * Interface send callback function.
 *
 * \param self    is the interface.
 * \param l2_addr identifies the target node on the attached subnet
 *   in case this is a subnet (PTMP) interface. Ignored otherwise.
 * \param msg     is the message to send.
 * \retval an error code.
 */
typedef int (*net_iface_send_f)(struct net_iface_t * self,
				net_addr_t l2_addr,
				net_msg_t * msg);

// -----[ net_iface_recv_f ]-----------------------------------------
/**
 * Interface receive callback function.
 *
 * \param self is the interface.
 * \param msg  is the received message.
 * \retval an error code.
 */
typedef int (*net_iface_recv_f)(struct net_iface_t * self,
				net_msg_t * msg);

// -----[ net_iface_destroy_f ]--------------------------------------
/**
 * Interface disposal callback function. This function is used to
 * free the memory reference by iface->user_data.
 */
typedef void (*net_iface_destroy_f)(void * ctx);

// ---[ Interface destination types ]---
/**
 * Definition of the target (the "other side") of a network
 * interface.
 *
 * An interface can be connected to another interface (RTR, PTP),
 * to a subnet (PTMP) or to a tunnel endpoint (VIRTUAL).
 */
typedef union {
  struct net_iface_t * iface;     /* rtr, ptp links */
  net_subnet_t       * subnet;    /* ptmp links */
  net_addr_t           end_point; /* tunnel endpoint */
} net_iface_dst_t;

// -----[ Network interface operations ]-----------------------------
/** Definition of the "virtual" methods of a network interface. */
typedef struct {
  net_iface_send_f    send;    /** Send message function. */
  net_iface_recv_f    recv;    /** Recv message function. */
  net_iface_destroy_f destroy; /** Message destroy function. */
} net_iface_ops_t;

// -----[ net_iface_phys_t ]-----------------------------------------
/** Definition of the physical characteristics of a network
 * interface. */
typedef struct {
  net_link_delay_t delay;    /** Propagation delay */
  net_link_load_t  capacity; /** Link capacity */
  net_link_load_t  load;     /** Link load */
} net_iface_phys_t;

// -----[ net_iface_t ]----------------------------------------------
/**
 * This type defines a network interface object.
 *
 * The interface data structure defines the \c owner node. The
 * interface is uniquely identified on this node by its IP address
 * (\c addr). The other side of the interface (the "device" to which
 * the interface is connected is defined by \c dest. The content of
 * \c dest depends on the interface type. This destination can
 * mainly take 3 different forms: point-to-point links (interface
 * is connected to another node's interface), point-to-multipoint
 * links (interface is connected to a subnet) and virtual links
 * (used for tunnels).
 *
 * In addition, the data structure holds the IGP weights assigned
 * to the interface. If the owner belongs to a multi-topology IGP
 * domain, there is one IGP weight per topology.
 *
 * Finally, the data structure maintains some informational data
 * such as physical attributes (delay, capacity, load).
 */
typedef struct net_iface_t {
  /** Attached node (owner). */
  net_node_t       * owner;

  /** Local interface address.
   *  \li ptp : remote destination addr,
   *  \li ptmp: local interface addr,
   *  \li tun : local interface addr
   */
  net_addr_t         addr;

  /** Local interface mask. ptp,tun: 32, ptmp: <32. */
  net_mask_t         mask;

  /** Tells if the interface is connected.
   * \todo should be moved in \c flags.*/
  uint8_t            connected;

  /** Type of interface. */
  net_iface_type_t   type;
  /** Interface's destination (depends on \c type). */
  net_iface_dst_t    dest;

  /** Physical attributes. */
  net_iface_phys_t   phys;
  /** Interface's state: (up/down), IGP_ADV */
  uint8_t            flags;
  /** List of IGP weights (1/topo). */
  igp_weights_t    * weights;
  /** User-defined data (depends on \c type). */
  void             * user_data;
  /** "Virtual" methods. */
  net_iface_ops_t    ops;        

#ifdef OSPF_SUPPORT
  /** Attached OSPF area. */
  ospf_area_t        area;
#endif
} net_iface_t;


//////////////////////////////////////////////////////////////////////
//
// MISC UTILITY TYPES DEFINITION
//
//////////////////////////////////////////////////////////////////////

// -----[ net_send_ctx_t ]-------------------------------------------
/**
 * This type defines a context record used to send messages. The
 * context holds the node to which the message must be delivered as
 * well as the message itself.
 * Upon delivery, the node's receive callback is called with the
 * recorded node as context.
 */
typedef struct {
  net_iface_t * dst_iface;
  net_msg_t   * msg;
} net_send_ctx_t;


// -----[ ip_trace_t ]-----------------------------------------------
/** Definition of an IP trace. */
typedef struct ip_trace_t {
  /** Array of traversed hops. */
  ptr_array_t       * items;
  /** QoS info: delay. */
  net_link_delay_t    delay;
  /** QoS info: IGP weight. */
  igp_weight_t        weight;
  /** QoS info: max capacity. */
  net_link_load_t     capacity;
  /** General status of trace. */
  net_error_t         status;
} ip_trace_t;


// -----[ net_elem_type_t ]------------------------------------------
/**
 * This type defines a union of network elements: iface, node, subnet,
 * IP trace, ...
 */
typedef enum {
  /** Element is a link. */
  LINK,
  /** Element is a node. */
  NODE,
  /** Element is a subnet. */
  SUBNET,
  /** Element is an IP trace. */
  TRACE,
} net_elem_type_t;


// -----[ net_elem_t ]-----------------------------------------------
/** Definition of a generic network element. */
typedef struct {
  /** Element type: iface, node, subnet, ... */
  net_elem_type_t   type;      
  /** Union of all element types. */
  union {
    /** Element as a link. */
    net_iface_t       * link;
    /** Element as a node. */
    net_node_t        * node;
    /** Element as a subnet. */
    net_subnet_t      * subnet;
    /** Element as an IP trace. */
    /*struct */ip_trace_t * trace;
  };
} net_elem_t;



/////////////////////////////////////////////////////////////////////
//
// ROUTING TABLE STRUCTURE
//
/////////////////////////////////////////////////////////////////////

// -----[ rt_entries_t ]---------------------------------------------
/** Definition of an array of routing table entries. */
typedef ptr_array_t rt_entries_t;




/////////////////////////////////////////////////////////////////////
//
// SPT STRUCTURE
//
/////////////////////////////////////////////////////////////////////

// -----[ spt_vertex_t ]---------------------------------------------
/**
 * Information stored in the Shortest-Paths Tree (SPT). For each
 * destination, the following information is stored:
 * - outgoing interface (on root node)
 * - gateway address
 * - weight
 */
GDS_ARRAY_TEMPLATE_TYPE(spt_vertices, struct spt_vertex_t *);
typedef struct spt_vertex_t {
  spt_vertices_t    * preds;
  spt_vertices_t    * succs;
  igp_weight_t        weight;
  net_elem_t          elem;
  ip_pfx_t            id;
  rt_entries_t      * rtentries;
} spt_vertex_t;
GDS_ARRAY_TEMPLATE_OPS(spt_vertices,spt_vertex_t *,
		       ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
		       _array_compare, NULL, NULL);

// -----[ spt_t ]----------------------------------------------------
typedef struct spt_t {
  gds_radix_tree_t * tree;
  spt_vertex_t     * root;
} spt_t;


#endif /* __NET_TYPES_H__ */
