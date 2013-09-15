// ==================================================================
// @(#)ipip.h
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 25/02/2004
// $Id: ipip.h,v 1.5 2009-03-24 16:14:28 bqu Exp $
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

#ifndef __NET_IPIP_H__
#define __NET_IPIP_H__

#include <net/message.h>
#include <net/network.h>
#include <net/prefix.h>
#include <net/protocol.h>

extern const net_protocol_def_t PROTOCOL_IPIP;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ ipip_link_create ]---------------------------------------
  int ipip_link_create(net_node_t * node, net_addr_t end_point,
		       net_addr_t addr, net_iface_t * oif,
		       net_addr_t src_addr, net_iface_t ** ppLink);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IPIP_H__ */
