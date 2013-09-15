// ==================================================================
// @(#)routing_t.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/02/2004
// $Id: routing_t.h,v 1.7 2009-03-24 16:25:54 bqu Exp $
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

#ifndef __NET_ROUTING_T_H__
#define __NET_ROUTING_T_H__

#include <libgds/array.h>
#include <libgds/trie.h>

/** Routes from any routing protocol. */
#define NET_ROUTE_ANY    0xFF
/** Direct routes. */
#define NET_ROUTE_DIRECT 0x01
/** Static routes. */
#define NET_ROUTE_STATIC 0x02
/** IGP routes. */
#define NET_ROUTE_IGP    0x04
/** BGP routes. */
#define NET_ROUTE_BGP    0x08

typedef uint8_t net_route_type_t;

typedef gds_trie_t net_rt_t;

#endif /* __NET_ROUTING_T_H__ */
