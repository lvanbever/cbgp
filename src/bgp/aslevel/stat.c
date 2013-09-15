// ==================================================================
// @(#)as-level-stat.c
//
// Provide structures and functions to report some AS-level
// topologies statistics.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/06/2007
// $Id: stat.c,v 1.1 2009-03-24 13:39:08 bqu Exp $
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
#include <string.h>

#include <bgp/as.h>
#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/types.h>

#define MAX_DEGREE MAX_AS

// -----[ aslevel_stat_degree ]--------------------------------------
void aslevel_stat_degree(gds_stream_t * stream, as_level_topo_t * topo,
			 int cumulated, int normalized, int inverse)
{
  unsigned int auDegreeFrequency[MAX_DEGREE];
  unsigned int index;
  unsigned int degree, max_degree= 0;
  double value, value2;
  as_level_domain_t * domain;
  unsigned int num_values;

  memset(auDegreeFrequency, 0, sizeof(auDegreeFrequency));
  
  num_values= 0;
  for (index= 0; index < aslevel_topo_num_nodes(topo); index++) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    degree= ptr_array_length(domain->neighbors);
    if (degree > max_degree)
      max_degree= degree;
    assert(degree < MAX_DEGREE);
    auDegreeFrequency[degree]++;
    num_values++;
  }

  value= 0;
  for (index= 0; index < max_degree; index++) {
    value2= auDegreeFrequency[index];
    if (normalized)
      value2= value2/(1.0*num_values);
    if (cumulated)
      value+= value2;
    else
      value= value2;
    if (inverse)
      stream_printf(stream, "%d\t%f\n", index, 1-value);
    else
      stream_printf(stream, "%d\t%f\n", index, value);
  }
}
