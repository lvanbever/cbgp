// ==================================================================
// @(#)export.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export.c,v 1.3 2009-03-24 16:07:06 bqu Exp $
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
#endif /* HAVE_CONFIG_H */

#include <assert.h>
#include <string.h>

#include <libgds/stream.h>

#include <net/error.h>
#include <net/export.h>
#include <net/export_cli.h>
#include <net/export_graphviz.h>
#include <net/export_ntf.h>
#include <net/network.h>

typedef net_error_t (*_net_export_fct_t)(gds_stream_t * stream,
					 network_t * network);

typedef struct {
  const char        * name;
  _net_export_fct_t   fct;
} _net_export_format_t;

static _net_export_format_t NET_EXPORT_FORMATS[NET_EXPORT_FORMAT_MAX]= {
  { "cli", net_export_cli },
  { "dot", net_export_dot },
  { "ntf", net_export_ntf },
};

// -----[ net_export_format2str ]------------------------------------
const char * net_export_format2str(net_export_format_t format)
{
  assert(format < NET_EXPORT_FORMAT_MAX);
  return NET_EXPORT_FORMATS[format].name;
}

// -----[ net_export_str2format ]------------------------------------
int net_export_str2format(const char * str,
			  net_export_format_t * format)
{
  *format= 0;
  while (*format < NET_EXPORT_FORMAT_MAX) {
    if (!strcmp(NET_EXPORT_FORMATS[*format].name, str))
      return ESUCCESS;
    (*format)++;
  }
  return EUNEXPECTED;
}

// -----[ net_export_stream ]----------------------------------------
/**
 *
 */
net_error_t net_export_stream(gds_stream_t * stream,
			      network_t * network,
			      net_export_format_t format)
{
  assert(format < NET_EXPORT_FORMAT_MAX);
  return NET_EXPORT_FORMATS[format].fct(stream, network);
}

// -----[ net_export_file ]------------------------------------------
/**
 *
 */
net_error_t net_export_file(const char * filename,
			    network_t * network,
			    net_export_format_t format)
{
  gds_stream_t * stream= stream_create_file(filename);
  int result;

  if (stream == NULL)
    return -1;

  result= net_export_stream(stream, network, format);

  stream_destroy(&stream);

  return result;
}
