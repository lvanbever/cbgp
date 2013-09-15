// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 23/02/2004
// $Id: message.h,v 1.8 2009-03-24 16:17:40 bqu Exp $
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

#ifndef __NET_MESSAGE_H__
#define __NET_MESSAGE_H__

#include <libgds/stream.h>
#include <net/prefix.h>
#include <net/net_types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- message_create -------------------------------------------
  net_msg_t * message_create(net_addr_t src_addr, net_addr_t dst_addr,
			     net_protocol_id_t proto, uint8_t ttl,
			     void * payload,
			     FPayLoadDestroy destroy);
  // ----- message_destroy ------------------------------------------
  void message_destroy(net_msg_t ** msg_ref);
  // -----[ message_set_options ]------------------------------------
  void message_set_options(net_msg_t * msg, struct ip_opt_t * opts);
  // -----[ message_copy ]-------------------------------------------
  net_msg_t * message_copy(net_msg_t * msg);
  // ----- message_dump ---------------------------------------------
  void message_dump(gds_stream_t * stream, net_msg_t * msg);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_MESSAGE_H__ */
