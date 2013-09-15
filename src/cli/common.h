// ==================================================================
// @(#)common.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/07/2003
// $Id: common.h,v 1.8 2009-08-24 10:33:30 bqu Exp $
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

#ifndef __CLI_COMMON_H__
#define __CLI_COMMON_H__

#include <libgds/cli.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ str2addr_id ]--------------------------------------------
  int str2addr_id(const char * id, net_addr_t * addr);
  // -----[ str2node ]-----------------------------------------------
  int str2node(const char * addr, net_node_t ** node);
  // -----[ str2node_id ]--------------------------------------------
  int str2node_id(const char * id, net_node_t ** node);
  // -----[ str2dest_id ]--------------------------------------------
  int str2dest_id(const char * str, ip_dest_t * dest);
  // -----[ str2subnet ]---------------------------------------------
  int str2subnet(const char * str, net_subnet_t ** subnet);

  // -----[ cli_arg_file ]-------------------------------------------
  cli_arg_t * cli_arg_file(const char * name, cli_arg_check_f check);
  // -----[ cli_opt_file ]---------------------------------------------
  cli_arg_t * cli_opt_file(const char * name, cli_arg_check_f check);

  // ----- cli_get --------------------------------------------------
  cli_t * cli_get();
  
  // -----[ _cli_common_init ]---------------------------------------
  void _cli_common_init();
  // ----- _cli_destroy ---------------------------------------------
  void _cli_common_destroy();

  // -----[ cli_set_param ]------------------------------------------
  void cli_set_param(const char * param, const char * value);
  // -----[ cli_get_param ]------------------------------------------
  const char * cli_get_param(const char * param);
  
#ifdef __cplusplus
}
#endif

#endif /* __CLI_COMMON_H__ */
