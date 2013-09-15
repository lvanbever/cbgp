// ==================================================================
// @(#)peer-list.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 10/03/2008
// $Id: peer-list.h,v 1.2 2009-03-24 14:28:25 bqu Exp $
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

#ifndef __BGP_PEER_LIST_H__
#define __BGP_PEER_LIST_H__

#include <libgds/array.h>
#include <libgds/enumerator.h>

#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_peers_create ]---------------------------------------
  bgp_peers_t * bgp_peers_create();
  // -----[ bgp_peers_destroy ]--------------------------------------
  void bgp_peers_destroy(bgp_peers_t ** peers);
  // -----[ bgp_peers_add ]------------------------------------------
  net_error_t bgp_peers_add(bgp_peers_t * peers, bgp_peer_t * peer);
  // -----[ bgp_peers_find ]-----------------------------------------
  bgp_peer_t * bgp_peers_find(bgp_peers_t * peers, net_addr_t addr);
  // -----[ bgp_peers_for_each ]-------------------------------------
  int bgp_peers_for_each(bgp_peers_t * peers, gds_array_foreach_f foreach,
			 void * ctx);
  // -----[ bgp_peers_enum ]-----------------------------------------
  gds_enum_t * bgp_peers_enum(bgp_peers_t * peers);

#ifdef __cplusplus
}
#endif

// -----[ bgp_peers_at ]---------------------------------------------
static inline bgp_peer_t * bgp_peers_at(bgp_peers_t * peers,
					unsigned int index) {
  return (bgp_peer_t *) peers->data[index];
}
// -----[ bgp_peers_size ]-------------------------------------------
static inline unsigned int bgp_peers_size(bgp_peers_t * peers) {
  return ptr_array_length(peers);
}


#endif /* __BGP_PEER_LIST_H__ */
