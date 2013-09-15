// ==================================================================
// @(#)export_cli.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export_cli.h,v 1.4 2009-03-24 16:07:49 bqu Exp $
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

#ifndef __NET_EXPORT_CLI_H__
#define __NET_EXPORT_CLI_H__

#include <libgds/stream.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_cli ]-----------------------------------------
  net_error_t net_export_cli(gds_stream_t * stream, network_t * network);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_CLI_H__ */
