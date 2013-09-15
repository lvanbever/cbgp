// ===========================================================
// @(#)link.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// $Id: link.h,v 1.23 2009-03-24 16:15:26 bqu Exp $
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
// ===========================================================

#ifndef __NET_LINK_H__
#define __NET_LINK_H__

#include <stdlib.h>

#include <net/iface.h>
#include <net/net_types.h>
#include <net/message.h>
#include <net/prefix.h>
#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/stream.h>
#include <libgds/memory.h>

#define NET_LINK_FLAG_UP      0x01

#define NET_LINK_MAX_DEPTH     10
#define NET_LINK_DEFAULT_DEPTH 1

#define NET_LINK_UNIDIR        0
#define NET_LINK_BIDIR         1

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_link_create_rtr ]------------------------------------
  int net_link_create_rtr(net_node_t * src_node, net_node_t * dst_node,
			  net_iface_dir_t dir, net_iface_t ** iface_ref);
  // -----[ net_link_create_ptp ]------------------------------------
  int net_link_create_ptp(net_node_t * src_node,
			  net_iface_id_t tSrcIfaceID,
			  net_node_t * dst_node,
			  net_iface_id_t tDstIfaceID,
			  net_iface_dir_t dir,
			  net_iface_t ** iface_ref);
  // -----[ net_link_create_ptmp ]-----------------------------------
  int net_link_create_ptmp(net_node_t * src_node, net_subnet_t * subnet,
			   net_addr_t iface_addr, net_iface_t ** iface_ref);
  // -----[ net_link_set_phys_attr ]---------------------------------
  int net_link_set_phys_attr(net_iface_t * iface, net_link_delay_t delay,
			     net_link_load_t capacity,
			     net_iface_dir_t dir);
  // ----- net_link_destroy -----------------------------------------
  void net_link_destroy(net_iface_t ** link_ref);


  ///////////////////////////////////////////////////////////////////
  // LINK DUMP
  ///////////////////////////////////////////////////////////////////

  // ----- net_link_dump --------------------------------------------
  void net_link_dump(gds_stream_t * stream, net_iface_t * link);
  // ----- net_link_dump_load ---------------------------------------
  void net_link_dump_load(gds_stream_t * stream, net_iface_t * link);
  // ----- net_link_dump_info ---------------------------------------
  void net_link_dump_info(gds_stream_t * stream, net_iface_t * link);


#ifdef __cplusplus
}
#endif

#endif /* __NET_LINK_H__ */
