// ==================================================================
// @(#)ntf.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/03/2005
// $Id: ntf.c,v 1.11 2009-03-24 16:08:30 bqu Exp $
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
#include <stdio.h>

#include <libgds/stream.h>

#include <net/network.h>
#include <net/node.h>
#include <net/ntf.h>
#include <net/util.h>
#include <util/lrp.h>


// -----[ MAX_NTF_LINE_LEN ]-----------------------------------------
/** Maximum number of characters in an NTF line.
 *
 * The minimum space needed is:
 *   = len(addr1)+1+len(addr2)+1+len(weight)+1+len(delay)+CR+LF+\0
 *   = 15+1+15+1+10+1+10+1+1+1
 *   = 56
 */
#define MAX_NTF_LINE_LEN 80

// -----[ NTF_FIELD_DELIMITERS ]-------------------------------------
#define NTF_FIELD_DELIMITERS " \t"


static lrp_t * _parser= NULL;


// -----[ _ntf_add_link ]--------------------------------------------
static inline
void _ntf_add_link(network_t * network,
		   net_addr_t addr1, net_addr_t addr2,
		   igp_weight_t weight, net_link_delay_t delay)
{
  net_node_t * node1, * node2;
  net_iface_t * iface;

  node1= network_find_node(network, addr1);
  if (node1 == NULL) {
    assert(node_create(addr1, &node1, NODE_OPTIONS_LOOPBACK) == ESUCCESS);
    assert(network_add_node(network, node1) == ESUCCESS);
  }
  
  node2= network_find_node(network, addr2);
  if (node2 == NULL) {
    assert(node_create(addr2, &node2, NODE_OPTIONS_LOOPBACK) == ESUCCESS);
    assert(network_add_node(network, node2) == ESUCCESS);
  }
    
  assert(net_link_create_rtr(node1, node2, BIDIR, &iface) == ESUCCESS);
  assert(net_link_set_phys_attr(iface, delay, 0, BIDIR) == ESUCCESS);
  assert(net_iface_set_metric(iface, 0, weight, BIDIR) == ESUCCESS);
}

// -----[ ntf_load ]-------------------------------------------------
int ntf_load(const char * filename)
{
  unsigned int num_fields;
  const char * field;
  net_addr_t addr1, addr2;
  igp_weight_t weight;
  net_link_delay_t delay;
  network_t * network= network_get_default();

  if (lrp_open(_parser, filename) < 0)
    return lrp_error(_parser);

  while (lrp_get_next_line(_parser)) {
    if (lrp_get_num_fields(_parser, &num_fields) < 0)
      break;
    
    // Check number of fields
    if ((num_fields < 3) || (num_fields > 4)) {
      lrp_set_user_error(_parser, "3 or 4 fields expected");
      break;
    }
    
    // Check IP address of first node
    field= lrp_get_field(_parser, 0);
    if (str2address(field, &addr1) < 0) {
      lrp_set_user_error(_parser, "invalid source \"%s\"", field);
      break;
    }
    
    // Check IP address of second node
    field= lrp_get_field(_parser, 1);
    if (str2address(field, &addr2) < 0) {
      lrp_set_user_error(_parser, "invalid destination \"%s\"", field);
      break;
    }
    
    // Check IGP metric
    field= lrp_get_field(_parser, 2);
    if (str2weight(field, &weight)) {
      lrp_set_user_error(_parser, "invalid IGP metric (%s)", field);
      break;
    }
    
    // Check delay (if provided)
    delay= 0;
    if (num_fields == 4) {
      field= lrp_get_field(_parser, 3);
      if (str2delay(field, &delay)) {
	lrp_set_user_error(_parser, "invalid delay (%s)", field);
	break;
      }
    }

    // Create link
    _ntf_add_link(network, addr1, addr2, weight, delay);
  }
  
  lrp_close(_parser);
  return lrp_error(_parser);
}

// -----[ ntf_save ]-------------------------------------------------
int ntf_save(const char * filename)
{
  return -1;
}

// -----[ ntf_strerror ]---------------------------------------------
const char * ntf_strerror()
{
  return lrp_strerror(_parser);
}

// -----[ ntf_strerrorloc ]------------------------------------------
const char * ntf_strerrorloc()
{
  return lrp_strerrorloc(_parser);
}

// -----[ _ntf_init ]------------------------------------------------
void _ntf_init()
{
  _parser= lrp_create(MAX_NTF_LINE_LEN, NTF_FIELD_DELIMITERS);
}

// -----[ _ntf_done ]------------------------------------------------
void _ntf_done()
{
  lrp_destroy(&_parser);
}
