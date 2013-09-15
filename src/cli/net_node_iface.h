// ==================================================================
// @(#)net_node_iface.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/02/2008
// $Id: net_node_iface.h,v 1.3 2009-03-24 15:58:43 bqu Exp $
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

#ifndef __CLI_NET_NODE_IFACE_H__
#define __CLI_NET_NODE_IFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_register_net_node_add_iface ]------------------------
  void cli_register_net_node_add_iface(cli_cmd_t * parent);
  // -----[ cli_register_net_node_iface ]----------------------------
  void cli_register_net_node_iface(cli_cmd_t * parent);

#ifdef __cplusplus
extern "C" {
#endif

#endif /* __CLI_NET_NODE_IFACE_H__ */
