// ==================================================================
// @(#)stats.c
//
// Network traffic statistics.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/12/2008
// $Id: stats.c,v 1.1 2009-08-31 15:04:20 bqu Exp $
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
#include <config.h>
#endif

#include <net/traffic/stats.h>

// -----[ flow_stats_init ]------------------------------------------
void flow_stats_init(flow_stats_t * stats)
{
  stats->flows_total= 0;
  stats->flows_ok   = 0;
  stats->flows_error= 0;
  stats->bytes_total= 0;
  stats->bytes_ok   = 0;
  stats->bytes_error= 0;
}

// -----[ flow_stats_dump ]------------------------------------------
void flow_stats_dump(gds_stream_t * stream,
		     flow_stats_t * stats)
{
  stream_printf(stream,
		"Flows total: %u\n"
		"Flows ok   : %u\n"
		"Flows error: %u\n"
		"Bytes total: %u\n"
		"Bytes ok   : %u\n"
		"Bytes error: %u\n",
		stats->flows_total, stats->flows_ok, stats->flows_error,
		stats->bytes_total, stats->bytes_ok, stats->bytes_error);
}

// -----[ flow_stats_count ]-----------------------------------------
void flow_stats_count(flow_stats_t * stats, unsigned int bytes)
{
  if (stats == NULL)
    return;
  stats->flows_total++;
  stats->bytes_total+= bytes;
}

// -----[ flow_stats_failure ]---------------------------------------
void flow_stats_failure(flow_stats_t * stats, unsigned int bytes)
{
  if (stats == NULL)
    return;
  stats->flows_error++;
  stats->bytes_error+= bytes;
}

// -----[ flow_stats_success ]---------------------------------------
void flow_stats_success(flow_stats_t * stats, unsigned int bytes)
{
  if (stats == NULL)
    return;
  stats->flows_ok++;
  stats->bytes_ok+= bytes;
}

