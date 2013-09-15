// ==================================================================
// @(#)protocol.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// $Id: protocol.h,v 1.7 2009-03-24 16:23:40 bqu Exp $
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

#ifndef __NET_PROTOCOL_H__
#define __NET_PROTOCOL_H__

#include <net/error.h>
#include <net/net_types.h>

extern const net_protocol_def_t * PROTOCOL_DEFS[NET_PROTOCOL_MAX];

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ protocol_recv ]------------------------------------------
  net_error_t protocol_recv(net_protocol_t * proto, net_msg_t * msg,
			    simulator_t * sim);
  
  // -----[ protocols_create ]---------------------------------------
  net_protocols_t * protocols_create();
  // -----[ protocols_destroy ]--------------------------------------
  void protocols_destroy(net_protocols_t ** protocols_ref);
  // -----[ protocols_add ]------------------------------------------
  int protocols_add(net_protocols_t * protocols, net_protocol_id_t id,
		    void * handler);
  // -----[ protocols_get ]------------------------------------------
  net_protocol_t * protocols_get(net_protocols_t * protocols,
				 net_protocol_id_t id);
  // -----[ net_protocol2str ]---------------------------------------
  const char * net_protocol2str(net_protocol_id_t id);
  // -----[ net_str2protocol ]---------------------------------------
  net_error_t net_str2protocol(const char * str, net_protocol_id_t * id);
  // -----[ net_protocols_get_def ]----------------------------------
  const net_protocol_def_t * net_protocols_get_def(net_protocol_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* __NET_PROTOCOL_H__ */
