// ==================================================================
// @(#)auto-config.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/11/2005
// $Id: auto-config.h,v 1.3 2009-03-24 13:56:45 bqu Exp $
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

#ifndef __BGP_AUTO_CONFIG_H__
#define __BGP_AUTO_CONFIG_H__

#include <net/prefix.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_auto_config_session ]--------------------------------
  int bgp_auto_config_session(bgp_router_t * router,
			      net_addr_t remote_addr,
			      uint16_t remote_asn,
			      bgp_peer_t ** peer_ref);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_AUTO_CONFIG_H__ */
