// ==================================================================
// @(#)iface_ptp.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// $Id: iface_ptp.h,v 1.3 2008-06-13 14:26:23 bqu Exp $
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

#ifndef __NET_IFACE_PTP_H__
#define __NET_IFACE_PTP_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_iface_new_ptp ]--------------------------------------
  net_iface_t * net_iface_new_ptp(net_node_t * node, ip_pfx_t pfx);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IFACE_PTP_H__ */
