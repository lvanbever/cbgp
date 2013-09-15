// ==================================================================
// @(#)export.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export.h,v 1.2 2009-03-24 16:07:06 bqu Exp $
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

#ifndef __NET_EXPORT_H__
#define __NET_EXPORT_H__

#include <libgds/stream.h>
#include <net/net_types.h>

typedef enum {
  NET_EXPORT_FORMAT_CLI,
  NET_EXPORT_FORMAT_DOT,
  NET_EXPORT_FORMAT_NTF,
  NET_EXPORT_FORMAT_MAX
} net_export_format_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_stream ]--------------------------------------
  int net_export_stream(gds_stream_t * stream,
			network_t * network,
			net_export_format_t format);
  // -----[ net_export_file ]----------------------------------------
  int net_export_file(const char * filename,
		      network_t * network,
		      net_export_format_t format);

  // -----[ net_export_format2str ]----------------------------------
  const char * net_export_format2str(net_export_format_t format);
  // -----[ net_export_str2format ]----------------------------------
  int net_export_str2format(const char * str,
			    net_export_format_t * format);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_H__ */
