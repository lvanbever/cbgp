// ==================================================================
// @(#)iface_rtr.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// $Id: iface_rtr.c,v 1.4 2009-03-24 16:11:45 bqu Exp $
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <net/iface.h>
#include <net/link.h>
#include <net/network.h>
#include <net/prefix.h>
#include <net/net_types.h>

// -----[ _net_iface_rtr_send ]--------------------------------------
/**
 * Forward a message along a point-to-point link. The next-hop
 * information is not used.
 */
static int _net_iface_rtr_send(net_iface_t * self,
			       net_addr_t l2_addr,
			       net_msg_t * msg)
{
  network_send(self->dest.iface, msg);
  return ESUCCESS;
}

// -----[ net_iface_new_rtr ]----------------------------------------
net_iface_t * net_iface_new_rtr(net_node_t * node, net_addr_t addr)
{
  net_iface_t * iface= net_iface_new(node, NET_IFACE_RTR);
  iface->addr= addr;
  iface->mask= 32;
  iface->ops.send= _net_iface_rtr_send;
  return iface;
}
