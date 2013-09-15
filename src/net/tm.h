// ==================================================================
// @(#)tm.h
//
// Traffic matrix management.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/01/2007
// $Id: tm.h,v 1.5 2009-08-31 09:48:28 bqu Exp $
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
 * Provide data structures and functions to parse and load a traffic
 * matrix out of a text file.
 *
 * The traffic matric file format is defined as follows. Each line
 * defines a traffic flow that enters the network through a node. The
 * flow enters the node through a specified network interface
 * identified by its IP address. The flow is destined to a
 * destination IP address or IP prefix. The flow record also defines
 * the flow volume in bytes (or whatever makes sense for the user).
 *
 * Each line contains 4 fields separated by a space or tabluation:
 *
 *   <node-id> <in-iface> <dst> <volume>
 *
 * where
 * \li <node-id> is the node's identifier (an IP address).
 * \li <in-iface> is the inbound network interface identifier
 *   (an IP address)
 * \li <dst is the flow destination (IP address or IP prefix).
 * \li <volume> is the flow volume.
 */

#ifndef __NET_TM_H__
#define __NET_TM_H__

#include <libgds/stream.h>

#include <util/lrp.h>
#include <bgp/types.h>

typedef struct {
  asn_t        src_asn;
  asn_t        dst_asn;
  net_addr_t   src_addr;
  net_addr_t   dst_addr;
  uint8_t      dst_mask;
  unsigned int bytes;
} flow_t;

// -----[ supported fields ]---------------------------------------
typedef enum {
  FLOW_FIELD_SRC_ASN,
  FLOW_FIELD_DST_ASN,
  FLOW_FIELD_SRC_IP,
  FLOW_FIELD_DST_IP,
  FLOW_FIELD_SRC_MASK,
  FLOW_FIELD_DST_MASK,
  FLOW_FIELD_OCTETS,
  FLOW_FIELD_PACKETS,
  FLOW_FIELD_SRC_PORT,
  FLOW_FIELD_DST_PORT,
  FLOW_FIELD_PROT,
  FLOW_FIELD_MAX
} flow_field_t;

// -----[ required fields ]------------------------------------------
typedef struct {
  uint8_t index[FLOW_FIELD_MAX];
} flow_field_map_t;

#define FLOW_FIELD_POS_REQUIRED 254
#define FLOW_FIELD_POS_IGNORE   255

static inline void flow_field_map_init(flow_field_map_t * map)
{
  unsigned int index;
  for (index= 0; index < FLOW_FIELD_MAX; index++)
    map->index[index]= FLOW_FIELD_POS_IGNORE;
}
static inline void flow_field_map_set(flow_field_map_t * map,
				      flow_field_t field)
{
  map->index[field]= FLOW_FIELD_POS_REQUIRED;
}
static inline int flow_field_map_isset(flow_field_map_t * map,
				       flow_field_t field)
{
  return ((map->index[field] != FLOW_FIELD_POS_REQUIRED) &&
	  (map->index[field] != FLOW_FIELD_POS_IGNORE));
}
static inline int flow_field_map_required(flow_field_map_t * map,
					  flow_field_t field)
{
  return (map->index[field] == FLOW_FIELD_POS_REQUIRED);
}

// -----[ flow_handler_f ]-------------------------------------------
typedef int (*flow_handler_f)(flow_t * flow, flow_field_map_t * map,
			      void * ctx);

// -----[ tm_error_t ]-----------------------------------------------
typedef enum {
  NET_TM_SUCCESS           = LRP_SUCCESS,
  NET_TM_ERROR             = LRP_ERROR_USER,
  NET_TM_ERROR_OPEN        = LRP_ERROR_USER-1,
  NET_TM_ERROR_NUM_PARAMS  = LRP_ERROR_USER-2,
  NET_TM_ERROR_INVALID_SRC = LRP_ERROR_USER-3,
  NET_TM_ERROR_UNKNOWN_SRC = LRP_ERROR_USER-4,
  NET_TM_ERROR_INVALID_DST = LRP_ERROR_USER-5,
  NET_TM_ERROR_INVALID_LOAD= LRP_ERROR_USER-6,
} tm_error_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ flow_str2field ]-----------------------------------------
  flow_field_t flow_str2field(const char * str);
  // -----[ flow_field2str ]-----------------------------------------
  const char * flow_field2str(flow_field_t field);

  // -----[ net_tm_strerror ]----------------------------------------
  char * net_tm_strerror(int error);
  // -----[ net_tm_perror ]------------------------------------------
  void net_tm_perror(gds_stream_t * stream, int error);
  // -----[ net_tm_parser ]------------------------------------------
  int net_tm_parser(FILE * stream);
  // -----[ net_tm_load ]--------------------------------------------
  int net_tm_load(const char * filename);

  // -----[ _tm_init ]-----------------------------------------------
  void _tm_init();
  // -----[ _tm_done ]-----------------------------------------------
  void _tm_done();

#ifdef __cplusplus
}
#endif

#endif /* __NET_TM_H__ */
