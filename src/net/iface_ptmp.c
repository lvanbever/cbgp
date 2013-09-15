// ==================================================================
// @(#)iface_ptmp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// $Id: iface_ptmp.c,v 1.7 2009-08-31 09:48:28 bqu Exp $
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

#include <assert.h>

#include <net/icmp_options.h>
#include <net/iface.h>
#include <net/link.h>
#include <net/network.h>
#include <net/prefix.h>
#include <net/net_types.h>
#include <net/subnet.h>

// -----[ _net_iface_ptmp_send ]-------------------------------------
static net_error_t _net_iface_ptmp_send(net_iface_t * self,
					net_addr_t l2_addr,
					net_msg_t * msg)
{
  net_subnet_t * subnet;
  net_iface_t * dst_iface;
  net_error_t error;
  int reached= 0;

  assert(self->type == NET_IFACE_PTMP);
  
  subnet= self->dest.subnet;

  error= ip_opt_hook_msg_subnet(subnet, msg, &reached);
  if (error != ESUCCESS)
    return error;
  if (reached) {
    message_destroy(&msg);
    return ESUCCESS;
  }

  // Find destination node (based on "physical address")
  dst_iface= net_subnet_find_link(subnet, l2_addr);
  if (dst_iface == NULL)
    return ENET_HOST_UNREACH;

  // Forward along link from subnet -> node ...
  if (!net_iface_is_enabled(dst_iface))
    return ENET_LINK_DOWN;

  network_send(dst_iface, msg);
  return ESUCCESS;
}

// -----[ net_iface_new_ptmp ]---------------------------------------
net_iface_t * net_iface_new_ptmp(net_node_t * node, ip_pfx_t pfx)
{
  net_iface_t * iface= net_iface_new(node, NET_IFACE_PTMP);
  iface->addr= pfx.network;
  iface->mask= pfx.mask;
  iface->ops.send= _net_iface_ptmp_send;
  return iface;
}



