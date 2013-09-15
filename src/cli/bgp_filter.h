// ==================================================================
// @(#)bgp_filter.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @date 27/02/2008
// $Id: bgp_filter.h,v 1.2 2009-03-24 15:58:13 bqu Exp $
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

#ifndef __CLI_BGP_FILTER_H__
#define __CLI_BGP_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_ctx_create_bgp_filter_add_rule -----------------------
  int cli_ctx_create_bgp_filter_add_rule(cli_ctx_t * ctx, cli_cmd_t * cmd,
					 void ** item);
  // ----- cli_ctx_create_bgp_filter_insert_rule --------------------
  int cli_ctx_create_bgp_filter_insert_rule(cli_ctx_t * ctx, cli_cmd_t * cmd,
					    void ** item);
  // ----- cli_bgp_filter_remove_rule -------------------------------
  int cli_bgp_filter_remove_rule(cli_ctx_t * ctx,
				 cli_cmd_t * cmd);
  // ----- cli_ctx_destroy_bgp_filter_rule --------------------------
  void cli_ctx_destroy_bgp_filter_rule(void ** item);
  // ----- cli_bgp_filter_show---------------------------------------
  int cli_bgp_filter_show(cli_ctx_t * ctx, cli_cmd_t * cmd);

  // -----[ cli_register_bgp_filter_rule ]---------------------------
  void * cli_register_bgp_filter_rule(cli_cmd_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_BGP_FILTER_H__ */
