// ==================================================================
// @(#)link_attr.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 12/01/2007
// $Id: link_attr.h,v 1.3 2009-03-24 16:17:11 bqu Exp $
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

#ifndef __NET_LINK_ATTR_H__
#define __NET_LINK_ATTR_H__

#include <libgds/array.h>
#include <net/net_types.h>

typedef uint32_t igp_weight_t;
typedef uint32_array_t igp_weights_t;

#define IGP_MAX_WEIGHT UINT32_MAX

//GDS_ARRAY_TEMPLATE(igp_weights, igp_weight_t, 0, NULL, NULL);

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_igp_weights_create ]---------------------------------
  igp_weights_t * net_igp_weights_create(uint8_t depth,
					 igp_weight_t dflt);
  // -----[ net_igp_add_weights ]------------------------------------
  igp_weight_t net_igp_add_weights(igp_weight_t weight1,
				   igp_weight_t weight2);

#ifdef __cplusplus
}
#endif


// -----[ net_igp_weights_destroy ]------------------------------------
static inline void net_igp_weights_destroy(igp_weights_t ** weights_ref)
{
  uint32_array_destroy(weights_ref);
}

// -----[ net_igp_weights_depth ]--------------------------------------
static inline unsigned int net_igp_weights_depth(igp_weights_t * weights)
{
  return uint32_array_size(weights);
}


#endif /* __NET_LINK_ATTR_H__ */
