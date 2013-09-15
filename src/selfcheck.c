// ==================================================================
// @(#)selfcheck.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 03/04/08
// $Id: selfcheck.c,v 1.5 2009-08-31 09:33:43 bqu Exp $
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
//
// Guidelines for writing C-BGP unit tests:
// ----------------------------------------
// - name according to object tested (e.g. test_bgp_router_xxx)
// - test functions must be defined with 'static' modifier (hence, the
//   compiler must warn us if the test is not referenced)
// - keep tests short
// - order tests according to object tested (hence according to
//   naming)
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/stream.h>
#include <libgds/hash_utils.h>
#include <libgds/str_util.h>
#include <libgds/utest.h>

#include <api.h>
#include <selfcheck.h>
#include <bgp/as.h>
#include <bgp/attr/comm.h>
#include <bgp/attr/comm_hash.h>
#include <bgp/attr/path.h>
#include <bgp/attr/path_hash.h>
#include <bgp/attr/path_segment.h>
#include <bgp/filter/filter.h>
#include <bgp/filter/parser.h>
#include <bgp/filter/predicate_parser.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <bgp/route-input.h>
#include <net/error.h>
#include <net/export.h>
#include <net/ez_topo.h>
#include <net/icmp.h>
#include <net/igp_domain.h>
#include <net/iface.h>
#include <net/igp.h>
#include <net/link-list.h>
#include <net/ipip.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/subnet.h>

static inline net_node_t * __node_create(net_addr_t addr) {
  net_node_t * node;
  assert(node_create(addr, &node, NODE_OPTIONS_LOOPBACK) == ESUCCESS);
  return node;
}

/////////////////////////////////////////////////////////////////////
//
// TEST TOPOLOGIES
//
/////////////////////////////////////////////////////////////////////

// -----[ _ez_topo_line_rtr ]----------------------------------------
static inline ez_topo_t * _ez_topo_line_rtr()
{
  ez_node_t nodes[]= {
    { .type=NODE, .domain=1 },
    { .type=NODE, .domain=1 },
  };
  ez_edge_t edges[]= {
    { .src=0, .dst=1, .weight=1 },
  };
  return ez_topo_builder(2, nodes, 1, edges);
}

// -----[ _ez_topo_line_ptp ]----------------------------------------
static inline ez_topo_t * _ez_topo_line_ptp()
{
  ez_node_t nodes[]= {
    { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
    { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
  };
  ez_edge_t edges[]= {
    { .src=0, .dst=1, .weight=1,
      .src_addr=IPV4(192,168,0,1),
      .dst_addr=IPV4(192,168,0,2) },
  };
  return ez_topo_builder(2, nodes, 1, edges);
}

// -----[ _ez_topo_triangle_rtr ]------------------------------------
static inline ez_topo_t * _ez_topo_triangle_rtr()
{
  ez_node_t nodes[]= {
    { .type=NODE, .domain=1 },
    { .type=NODE, .domain=1 },
    { .type=NODE, .domain=1 },
  };
  ez_edge_t edges[]= {
    { .src=0, .dst=1, .weight=10, .delay=52, .capacity=256, },
    { .src=0, .dst=2, .weight=1, .delay=13, .capacity=1024, },
    { .src=1, .dst=2, .weight=1, .delay=26, .capacity=512 },
  };
  return ez_topo_builder(3, nodes, 3, edges);
}

// -----[ _ez_topo_triangle_ptp ]------------------------------------
static inline ez_topo_t * _ez_topo_triangle_ptp()
{
  ez_node_t nodes[]= {
    { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
    { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
    { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
  };
  ez_edge_t edges[]= {
    { .src=0, .dst=1, .weight=10,
      .src_addr=IPV4(192,168,0,1), .dst_addr=IPV4(192,168,0,2) },
    { .src=0, .dst=2, .weight=1,
      .src_addr=IPV4(192,168,1,1), .dst_addr=IPV4(192,168,1,2) },
    { .src=1, .dst=2, .weight=1,
      .src_addr=IPV4(192,168,2,1), .dst_addr=IPV4(192,168,2,2) },
  };
  return ez_topo_builder(3, nodes, 3, edges);
}

// -----[ _ez_topo_square ]------------------------------------------
static inline ez_topo_t * _ez_topo_square()
{
  ez_node_t nodes[]= {
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
  };
  ez_edge_t edges[]= {
    { .src=0, .dst=1, .weight=1 },
    { .src=0, .dst=2, .weight=1 },
    { .src=1, .dst=3, .weight=1 },
    { .src=2, .dst=3, .weight=1 },
  };
  return ez_topo_builder(4, nodes, 4, edges);
}

// -----[ _ez_topo_glasses ]-----------------------------------------
static inline ez_topo_t * _ez_topo_glasses()
{
  ez_node_t nodes[]= {
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 }
  };
  ez_edge_t edges[]= {
    { .src=0, .dst=1, .weight=1 },
    { .src=0, .dst=2, .weight=1 },
    { .src=1, .dst=3, .weight=1 },
    { .src=2, .dst=3, .weight=1 },
    { .src=3, .dst=4, .weight=2 },
    { .src=3, .dst=5, .weight=3 },
    { .src=4, .dst=6, .weight=2 },
    { .src=5, .dst=6, .weight=1 }
  };
  return ez_topo_builder(7, nodes, 8, edges);
}


/////////////////////////////////////////////////////////////////////
//
// SIMULATOR
//
/////////////////////////////////////////////////////////////////////

#define SIM_ARRAY_SIZE 2
static unsigned int _sim_array_index;
static void * _sim_array[SIM_ARRAY_SIZE];
static int _sim_callback(simulator_t * sim, void * ctx)
{
  if (_sim_array_index >= SIM_ARRAY_SIZE)
    return -1;
  _sim_array[_sim_array_index++]= ctx;
  return 0;
}

// -----[ test_sim_static ]------------------------------------------
static int test_sim_static()
{
  sim_event_ops_t ops= { .callback= _sim_callback,
			 .destroy= NULL,
			 .dump= NULL };
  simulator_t * sim= sim_create(SCHEDULER_STATIC);
  UTEST_ASSERT(sim_get_num_events(sim) == 0, "should return 0 events");
  UTEST_ASSERT(sim_get_time(sim) == 0, "should return time 0.0");
  UTEST_ASSERT(sim_post_event(sim, &ops, (void *) 1234, 0, SIM_TIME_REL) == 0,
	       "sim_post_event() should succeed");
  UTEST_ASSERT(sim_get_num_events(sim) == 1, "should return 1 event");
  UTEST_ASSERT(sim_get_time(sim) == 0, "should return time 0.0");
  UTEST_ASSERT(sim_post_event(sim, &ops, (void *) 2345, 0, SIM_TIME_REL) == 0,
	       "sim_post_event() should succeed");
  UTEST_ASSERT(sim_get_num_events(sim) == 2, "should return 2 events");
  UTEST_ASSERT(sim_get_time(sim) == 0, "should return time 0.0");
  _sim_array_index= 0;
  UTEST_ASSERT(sim_run(sim) == 0, "sim_run() should succeed");
  UTEST_ASSERT(_sim_array_index == SIM_ARRAY_SIZE,
	       "some events were not processed");
  UTEST_ASSERT((_sim_array[0] == (void *) 1234) &&
	       (_sim_array[1] == (void *) 2345), "incorrect events processing");
  sim_destroy(&sim);
  UTEST_ASSERT(sim == NULL, "destroyed simulator should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_sim_static_clear ]------------------------------------
static int test_sim_static_clear()
{
  simulator_t * sim= sim_create(SCHEDULER_STATIC);
  sim_post_event(sim, NULL, (void *) 1234, 0, SIM_TIME_REL);
  sim_post_event(sim, NULL, (void *) 2345, 0, SIM_TIME_REL);
  sim_clear(sim);
  UTEST_ASSERT(sim_get_num_events(sim) == 0, "should return 0 events");
  sim_destroy(&sim);
  return UTEST_SUCCESS;
}

// -----[ test_sim_dynamic ]-----------------------------------------
static int test_sim_dynamic()
{
  sim_event_ops_t ops= { .callback= _sim_callback,
			 .destroy= NULL,
			 .dump= NULL };
  simulator_t * sim= sim_create(SCHEDULER_DYNAMIC);
  UTEST_ASSERT(sim_get_num_events(sim) == 0, "should return 0 events");
  UTEST_ASSERT(sim_get_time(sim) == 0, "should return time 0.0");
  UTEST_ASSERT(sim_post_event(sim, &ops, (void *) 1234, 12, SIM_TIME_REL) == 0,
	       "sim_post_event() should succeed");
  UTEST_ASSERT(sim_get_num_events(sim) == 1, "should return 1 event");
  UTEST_ASSERT(sim_get_time(sim) == 0, "should return time 0.0");
  UTEST_ASSERT(sim_post_event(sim, &ops, (void *) 2345, 6, SIM_TIME_REL) == 0,
	       "sim_post_event() should succeed");
  UTEST_ASSERT(sim_get_num_events(sim) == 2, "should return 2 events");
  UTEST_ASSERT(sim_get_time(sim) == 0, "should return time 0.0");
  _sim_array_index= 0;
  UTEST_ASSERT(sim_run(sim) == 0, "sim_run() should succeed");
  UTEST_ASSERT(_sim_array_index == SIM_ARRAY_SIZE,
	       "some events were not processed");
  UTEST_ASSERT((_sim_array[0] == (void *) 2345) &&
	       (_sim_array[1] == (void *) 1234), "incorrect events processing");
  sim_destroy(&sim);
  UTEST_ASSERT(sim == NULL, "destroyed simulator should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_sim_dynamic_clear ]-----------------------------------
static int test_sim_dynamic_clear()
{
  simulator_t * sim= sim_create(SCHEDULER_DYNAMIC);
  sim_post_event(sim, NULL, (void *) 1234, 12, SIM_TIME_REL);
  sim_post_event(sim, NULL, (void *) 2345, 6, SIM_TIME_REL);
  sim_clear(sim);
  UTEST_ASSERT(sim_get_num_events(sim) == 0, "should return 0 events");
  sim_destroy(&sim);
  return UTEST_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
//
// NET ATTRIBUTES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_attr_address4 ]-----------------------------------
static int test_net_attr_address4()
{
  net_addr_t addr= IPV4(255,255,255,255);
  UTEST_ASSERT(addr == UINT32_MAX,
		"255.255.255.255 should be equal to %u", UINT32_MAX);
  addr= NET_ADDR_ANY;
  UTEST_ASSERT(addr == 0,
		"0.0.0.0 should be equal to 0");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_address4_2str ]------------------------------
static int test_net_attr_address4_2str()
{
  char acBuffer[16];
  UTEST_ASSERT(ip_address_to_string(IPV4(192,168,0,0),
				     acBuffer, sizeof(acBuffer)) == 11,
		"ip_address_to_string() should return 11");
  UTEST_ASSERT(strcmp(acBuffer, "192.168.0.0") == 0,
		"string for address 192.168.0.0 should be \"192.168.0.0\"");
  UTEST_ASSERT(ip_address_to_string(IPV4(192,168,1,23),
				     acBuffer, sizeof(acBuffer)) == 12,
		"ip_address_to_string() should return 12");
  UTEST_ASSERT(strcmp(acBuffer, "192.168.1.23") == 0,
		"string for address 192.168.1.23 should be \"192.168.1.23\"");
  UTEST_ASSERT(ip_address_to_string(IPV4(255,255,255,255),
				     acBuffer, sizeof(acBuffer)-1) < 0,
		"ip_address_to_string() should return < 0");
  UTEST_ASSERT(ip_address_to_string(IPV4(255,255,255,255),
				     acBuffer, sizeof(acBuffer)) == 15,
		"ip_address_to_string() should return 15");
  UTEST_ASSERT(strcmp(acBuffer, "255.255.255.255") == 0,
		"string for address 255.255.255.255 should be \"255.255.255.255\"");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_address4_str2 ]------------------------------
static int test_net_attr_address4_str2()
{
  net_addr_t addr;
  char * pcEndPtr;
  UTEST_ASSERT((ip_string_to_address("192.168.0.0", &pcEndPtr, &addr) >= 0) && (*pcEndPtr == '\0'),
		"address \"192.168.0.0\" should be valid");
  UTEST_ASSERT(addr == IPV4(192,168,0,0),
		"address should 192.168.0.0");
  UTEST_ASSERT((ip_string_to_address("192.168.1.23", &pcEndPtr, &addr) >= 0) && (*pcEndPtr == '\0'),
		"address \"192.168.1.23\" should be valid");
  UTEST_ASSERT(addr == IPV4(192,168,1,23),
		"address should 192.168.1.23");
  UTEST_ASSERT((ip_string_to_address("255.255.255.255", &pcEndPtr, &addr) >= 0) && (*pcEndPtr == '\0'),
		"address \"255.255.255.255\" should be valid");
  UTEST_ASSERT(addr == IPV4(255,255,255,255),
		"address should 255.255.255.255");
  UTEST_ASSERT((ip_string_to_address("64.233.183.147a", &pcEndPtr, &addr) >= 0) && (*pcEndPtr == 'a'),
		"address \"64.233.183.147a\" should be valid (followed by stuff)");
  UTEST_ASSERT(addr == IPV4(64,233,183,147),
		"address should 64.233.183.147");
  UTEST_ASSERT(ip_string_to_address("64a.233.183.147", &pcEndPtr, &addr) < 0,
		"address \"64a.233.183.147a\" should be invalid");
  UTEST_ASSERT(ip_string_to_address("255.256.0.0", &pcEndPtr, &addr) < 0,
		"address \"255.256.0.0\" should be invalid");
  UTEST_ASSERT((ip_string_to_address("255.255.0.0.255", &pcEndPtr, &addr) >= 0) && (*pcEndPtr != '\0'),
		"address \"255.255.0.0.255\" should be valid (followed by stuff)");
  UTEST_ASSERT(ip_string_to_address("255.255.0", &pcEndPtr, &addr) < 0,
		"address \"255.255.0.0.255\" should be invalid");
  UTEST_ASSERT(ip_string_to_address("255..255.0", &pcEndPtr, &addr) < 0,
		"address \"255..255.0\" should be invalid");
  UTEST_ASSERT((ip_string_to_address("1.2.3.4.5", &pcEndPtr, &addr) == 0) &&
		(*pcEndPtr == '.'),
		"address \"1.2.3.4.5\" should be invalid");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4 ]------------------------------------
static int test_net_attr_prefix4()
{
  ip_pfx_t pfx= IPV4PFX(255,255,255,255,32);
  ip_pfx_t * pfx_ptr;
  UTEST_ASSERT((pfx.network == UINT32_MAX),
		"255.255.255.255 should be equal to %u", UINT32_MAX);
  pfx.network= NET_ADDR_ANY;
  pfx.mask= 0;
  UTEST_ASSERT(pfx.network == 0,
		"0.0.0.0 should be equal to 0");

  pfx_ptr= create_ip_prefix(IPV4(1,2,3,4), 5);
  UTEST_ASSERT((pfx_ptr != NULL) &&
		(pfx_ptr->network == IPV4(1,2,3,4)) &&
		(pfx_ptr->mask == 5),
		"prefix should be 1.2.3.4/5");
  ip_prefix_destroy(&pfx_ptr);
  UTEST_ASSERT(pfx_ptr == NULL,
		"destroyed prefix should be null");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_in ]---------------------------------
static int test_net_attr_prefix4_in()
{
  ip_pfx_t pfx1= IPV4PFX(192,168,1,0, 24);
  ip_pfx_t pfx2= IPV4PFX(192,168,0,0,16);
  ip_pfx_t pfx3= IPV4PFX(192,168,2,0,24);
  UTEST_ASSERT(ip_prefix_in_prefix(pfx1, pfx2) != 0,
		"192.168.1/24 should be in 192.168/16");
  UTEST_ASSERT(ip_prefix_in_prefix(pfx3, pfx2) != 0,
		"192.168.2/24 should be in 192.168/16");
  UTEST_ASSERT(ip_prefix_in_prefix(pfx2, pfx1) == 0,
		"192.168/16 should not be in 192.168.1/24");
  UTEST_ASSERT(ip_prefix_in_prefix(pfx3, pfx1) == 0,
		"192.168.2/24 should not be in 192.168.1/24");
  UTEST_ASSERT(ip_prefix_in_prefix(pfx1, pfx3) == 0,
		"192.168.1/24 should not be in 192.168.2/24");
  UTEST_ASSERT(ip_prefix_in_prefix(pfx1, pfx1) != 0,
		"192.168.1/24 should be in 192.168.1/24");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_ge ]---------------------------------
static int test_net_attr_prefix4_ge()
{
  ip_pfx_t pfx1= IPV4PFX(192,168,0,0,16);
  ip_pfx_t pfx2= IPV4PFX(192,168,1,0,24);
  ip_pfx_t pfx3= IPV4PFX(192,168,0,0,15);
  ip_pfx_t pfx4= IPV4PFX(192,169,0,0,24);
  UTEST_ASSERT(ip_prefix_ge_prefix(pfx1, pfx1, 16),
		"192.168/16 should be ge (192.168/16, 16)");
  UTEST_ASSERT(ip_prefix_ge_prefix(pfx2, pfx1, 16),
		"192.168.1/24 should be ge (192.168/16, 16)");
  UTEST_ASSERT(!ip_prefix_ge_prefix(pfx3, pfx1, 16),
		"192.168/15 should not be ge (192.168/16, 16)");
  UTEST_ASSERT(!ip_prefix_ge_prefix(pfx4, pfx1, 16),
		"192.169.0/24 should not be ge (192.168/16, 16)");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_le ]---------------------------------
static int test_net_attr_prefix4_le()
{
  ip_pfx_t pfx1= IPV4PFX(192,168,0,0,16);
  ip_pfx_t pfx2= IPV4PFX(192,168,1,0,24);
  ip_pfx_t pfx3= IPV4PFX(192,168,1,0,25);
  ip_pfx_t pfx4= IPV4PFX(192,169,0,0,24);
  ip_pfx_t pfx5= IPV4PFX(192,168,0,0,15);
  UTEST_ASSERT(ip_prefix_le_prefix(pfx2, pfx1, 24),
		"192.168.1/24 should be le (192.168/16, 24)");
  UTEST_ASSERT(!ip_prefix_le_prefix(pfx3, pfx1, 24),
		"192.168.0.0/25 should not be le (192.168/16, 24)");
  UTEST_ASSERT(!ip_prefix_le_prefix(pfx4, pfx1, 24),
		"192.169/24 should not be le (192.168/16, 24)");
  UTEST_ASSERT(ip_prefix_le_prefix(pfx1, pfx1, 24),
		"192.168/16 should be le (192.168/16, 24)");
  UTEST_ASSERT(!ip_prefix_le_prefix(pfx5, pfx1, 24),
		"192.168/15 should not be le (192.168/16, 24)");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_match ]------------------------------
static int test_net_attr_prefix4_match()
{
  ip_pfx_t pfx= IPV4PFX(0,0,0,0,0);
  UTEST_ASSERT(ip_address_in_prefix(IPV4(0,0,0,0), pfx) != 0,
		"0.0.0.0 should be in 0.0.0.0/0");
  UTEST_ASSERT(ip_address_in_prefix(IPV4(255,255,255,255), pfx) != 0,
		"255.255.255.255 should be in 0.0.0.0/0");
  pfx= IPV4PFX(255,255,255,255,32);
  UTEST_ASSERT(ip_address_in_prefix(IPV4(0,0,0,0), pfx) == 0,
		"0.0.0.0 should not be in 255.255.255.255/32");
  UTEST_ASSERT(ip_address_in_prefix(IPV4(255,255,255,255), pfx) != 0,
		"255.255.255.255 should be in 255.255.255.255/32");
  pfx= IPV4PFX(130,104,229,224,27);
  UTEST_ASSERT(ip_address_in_prefix(IPV4(130,104,229,225), pfx) != 0,
		"130.104.229.225 should be in 130.104.229.224/27");
  UTEST_ASSERT(ip_address_in_prefix(IPV4(130,104,229,240), pfx) != 0,
		"130.104.229.240 should be in 130.104.229.224/27");
  UTEST_ASSERT(ip_address_in_prefix(IPV4(130,104,229,256), pfx) == 0,
		"130.104.229.256 should not be in 130.104.229.224/27");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_mask ]-------------------------------
static int test_net_attr_prefix4_mask()
{
  ip_pfx_t pfx= IPV4PFX(192,168,128,2,17);
  ip_prefix_mask(&pfx);
  UTEST_ASSERT((pfx.network == IPV4(192,168,128,0)) &&
		(pfx.mask == 17),
		"masked prefix should be 192.168.128.0/17");
  pfx= IPV4PFX(255,255,255,255,1);
  ip_prefix_mask(&pfx);
  UTEST_ASSERT((pfx.network == IPV4(128,0,0,0)) &&
		(pfx.mask == 1),
		"masked prefix should be 128.0.0.0/1");
  pfx= IPV4PFX(255,255,255,255,32);
  ip_prefix_mask(&pfx);
  UTEST_ASSERT((pfx.network == IPV4(255,255,255,255)) &&
		(pfx.mask == 32),
		"masked prefix should be 255.255.255.255/32");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_2str ]-------------------------------
static int test_net_attr_prefix4_2str()
{
  ip_pfx_t pfx= IPV4PFX(192,168,0,0,16);
  char acBuffer[19];
  UTEST_ASSERT(ip_prefix_to_string(&pfx, acBuffer, sizeof(acBuffer))
		== 14,
		"ip_prefix_to_string() should return 14");
  UTEST_ASSERT(strcmp(acBuffer, "192.168.0.0/16") == 0,
		"string for prefix 192.168/16 should be \"192.168.0.0/16\"");
  pfx= IPV4PFX(255,255,255,255,32);
  UTEST_ASSERT(ip_prefix_to_string(&pfx, acBuffer, sizeof(acBuffer)-1)
		< 0,
		"ip_prefix_to_string() should return < 0");
  UTEST_ASSERT(ip_prefix_to_string(&pfx, acBuffer, sizeof(acBuffer))
		== 18,
		"ip_prefix_to_string() should return 18");
  UTEST_ASSERT(strcmp(acBuffer, "255.255.255.255/32") == 0,
		"string for prefix 255.255.255.255/32 should "
		"be \"255.255.255.255/32\"");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_str2 ]-------------------------------
static int test_net_attr_prefix4_str2()
{
  ip_pfx_t pfx;
  char * pcEndPtr;
  UTEST_ASSERT((ip_string_to_prefix("192.168.0.0/16", &pcEndPtr, &pfx) >= 0) && (*pcEndPtr == '\0'),
		"prefix \"192.168.0.0/16\" should be valid");
  UTEST_ASSERT((pfx.network == IPV4(192,168,0,0)) &&
		(pfx.mask == 16),
		"prefix should be 192.168.0.0/16");
  UTEST_ASSERT((ip_string_to_prefix("192.168/16", &pcEndPtr, &pfx) >= 0) && (*pcEndPtr == '\0'),
		"prefix \"192.168/16\" should be valid");
  UTEST_ASSERT((pfx.network == IPV4(192,168,0,0)) &&
		(pfx.mask == 16),
		"prefix should be 192.168.0.0/16");
  UTEST_ASSERT((ip_string_to_prefix("192.168.1.2/28", &pcEndPtr, &pfx) >= 0) && (*pcEndPtr == '\0'),
		"prefix \"192.168.1.2/28\" should be valid");
  UTEST_ASSERT((pfx.network == IPV4(192,168,1,2)) &&
		(pfx.mask == 28),
		"prefix should be 192.168.1.2/28");
  UTEST_ASSERT((ip_string_to_prefix("255.255.255.255/32", &pcEndPtr, &pfx) >= 0) && (*pcEndPtr == '\0'),
		"prefix \"255.255.255.255/32\" should be valid");
  UTEST_ASSERT((pfx.network == IPV4(255,255,255,255)) &&
		(pfx.mask == 32),
		"prefix should be 255.255.255.255/32");
  UTEST_ASSERT((ip_string_to_prefix("192.168.1.2/16a", &pcEndPtr, &pfx) >= 0) && (*pcEndPtr == 'a'),
		"prefix should be valid (but followed by stuff)");
  UTEST_ASSERT((ip_string_to_prefix("192.168a.1.2/16", &pcEndPtr, &pfx) < 0),
		"prefix should be invalid");
  UTEST_ASSERT((ip_string_to_prefix("256/8", &pcEndPtr, &pfx) < 0),
		"prefix should be invalid");
  UTEST_ASSERT((ip_string_to_prefix("255.255.255.255.255/8", &pcEndPtr, &pfx) < 0),
		"prefix should be invalid");
  UTEST_ASSERT((ip_string_to_prefix("255.255.255.255/99", &pcEndPtr, &pfx) < 0),
		"prefix should be invalid");
  UTEST_ASSERT((ip_string_to_prefix("255.255", &pcEndPtr, &pfx) < 0),
		"prefix should be invalid");
  UTEST_ASSERT((ip_string_to_prefix("255.255./16", &pcEndPtr, &pfx) < 0),
		"prefix should be invalid");
  UTEST_ASSERT((ip_string_to_prefix("1.2.3.4.5", &pcEndPtr, &pfx) < 0),
		"prefix \"1.2.3.4.5\" should be invalid");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_prefix4_cmp ]--------------------------------
static int test_net_attr_prefix4_cmp()
{
  ip_pfx_t pfx1= IPV4PFX(1,2,3,4,24);
  ip_pfx_t pfx2= pfx1;
  UTEST_ASSERT(ip_prefix_cmp(&pfx1, &pfx2) == 0,
		"1.2.3.4/24 should be equal to 1.2.3.4.24");
  pfx2.network= IPV4(1,2,2,4);
  UTEST_ASSERT(ip_prefix_cmp(&pfx1, &pfx2) == 1,
		"1.2.3.4/24 should be > than 1.2.2.4/24");
  pfx2.network= pfx1.network;
  pfx2.mask= 25;
  UTEST_ASSERT(ip_prefix_cmp(&pfx1, &pfx2) == -1,
		"1.2.3.4/24 should be > than 1.2.3.4/25");
  pfx1= IPV4PFX(1,2,3,1,24);
  pfx2= IPV4PFX(1,2,3,0,24);
  UTEST_ASSERT(ip_prefix_cmp(&pfx1, &pfx2) == 0,
		"1.2.3.1/24 should be > than 1.2.3.0/24");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_dest ]---------------------------------------
static int test_net_attr_dest()
{
  ip_dest_t dest;
  dest.type= NET_DEST_ADDRESS;
  dest.addr= IPV4(192,168,0,1);
  UTEST_ASSERT(dest.type == NET_DEST_ADDRESS,
		"dest type should be ADDRESS");
  UTEST_ASSERT(dest.addr == IPV4(192,168,0,1),
		"dest address should be 192.168.0.1");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_dest_str2 ]----------------------------------
static int test_net_attr_dest_str2()
{
  ip_dest_t dest;
  UTEST_ASSERT(ip_string_to_dest("*", &dest) == 0,
		"destination should be valid");
  UTEST_ASSERT(dest.type == NET_DEST_ANY,
		"conversion is invalid");
  UTEST_ASSERT(ip_string_to_dest("1.2.3.4", &dest) == 0,
		"destination should be valid"); 
  UTEST_ASSERT((dest.type == NET_DEST_ADDRESS) &&
		(dest.addr == IPV4(1,2,3,4)),
		"conversion is invalid");
  UTEST_ASSERT(ip_string_to_dest("1.2.3.4/25", &dest) == 0,
		"destination should be valid"); 
  UTEST_ASSERT((dest.type == NET_DEST_PREFIX) &&
		(dest.prefix.network == IPV4(1,2,3,4)) &&
		(dest.prefix.mask == 25),
		"conversion is invalid");
  UTEST_ASSERT(ip_string_to_dest("1.2.3.4/33", &dest) < 0,
		"destination should be invalid");
  UTEST_ASSERT(ip_string_to_dest("1.2.3.4.5", &dest) < 0,
		"destination should be invalid");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_igpweight ]----------------------------------
static int test_net_attr_igpweight()
{
  igp_weights_t * weights= net_igp_weights_create(2, IGP_MAX_WEIGHT);
  UTEST_ASSERT(weights != NULL,
		"net_igp_weights_create() should not return NULL");
  UTEST_ASSERT(net_igp_weights_depth(weights) == 2,
		"IGP weights'depth should be 2");
  net_igp_weights_destroy(&weights);
  UTEST_ASSERT(weights == NULL,
		"destroyed IGP weights should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_attr_igpweight_add ]------------------------------
static int test_net_attr_igpweight_add()
{
  UTEST_ASSERT(net_igp_add_weights(5, 10) == 15,
		"IGP weights 5 and 10 should add to 15");
  UTEST_ASSERT(net_igp_add_weights(5, IGP_MAX_WEIGHT)
		== IGP_MAX_WEIGHT,
		"IGP weights 5 and 10 should add to %d", IGP_MAX_WEIGHT);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET NODES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_node ]--------------------------------------------
static int test_net_node()
{
  net_node_t * node;
  UTEST_ASSERT(node_create(IPV4(1,0,0,0), &node, NODE_OPTIONS_LOOPBACK)
		== ESUCCESS,
		"node creation should succeed");
  UTEST_ASSERT(node != NULL, "node creation should succeed");
  UTEST_ASSERT(node->rid == IPV4(1,0,0,0),
		"incorrect node address (ID)");
  UTEST_ASSERT(node->ifaces != NULL,
		"interfaces list should be created");
  UTEST_ASSERT(node->protocols != NULL,
		"protocols list should be created");
  UTEST_ASSERT(node->rt != NULL,
		"forwarding table should be created");
  node_destroy(&node);
  UTEST_ASSERT(node == NULL, "destroyed node should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_node_0 ]------------------------------------------
static int test_net_node_0()
{
  net_node_t * node;
  UTEST_ASSERT(node_create(NET_ADDR_ANY, &node, NODE_OPTIONS_LOOPBACK)
		!= ESUCCESS,
		"it should not be possible to create node 0.0.0.0");
  return UTEST_SUCCESS;
}

// -----[ test_net_node_name ]---------------------------------------
static int test_net_node_name()
{
  net_node_t * node= __node_create(IPV4(1,0,0,0));
  UTEST_ASSERT(node_get_name(node) == NULL,
		"node should not have a default name");
  node_set_name(node, "NODE 1");
  UTEST_ASSERT(!strcmp(node_get_name(node), "NODE 1"),
		"node should have name \"NODE 1\"");
  node_set_name(node, NULL);
  UTEST_ASSERT(node_get_name(node) == NULL,
		"node should not have a name");
  node_destroy(&node);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET SUBNETS
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_subnet ]------------------------------------------
static int test_net_subnet()
{
  net_subnet_t * subnet= subnet_create(IPV4(192,168,0,0), 16,
				      NET_SUBNET_TYPE_STUB);
  UTEST_ASSERT(subnet != NULL, "subnet creation should succeed");
  subnet_destroy(&subnet);
  UTEST_ASSERT(subnet == NULL, "destroyed subnet should be NULL");
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET INTERFACES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_iface_lo ]----------------------------------------
static int test_net_iface_lo()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  net_addr_t addr= IPV4(172,0,0,1);
  net_iface_t * iface= NULL;
  UTEST_ASSERT(net_iface_factory(node, net_iface_id_addr(addr),
				 NET_IFACE_LOOPBACK, &iface) == ESUCCESS,
	       "should be able to create loopback interface");
  UTEST_ASSERT(iface->type == NET_IFACE_LOOPBACK,
	       "interface type should be LOOPBACK");
  UTEST_ASSERT(iface->addr == addr,
	       "incorrect interface address");
  UTEST_ASSERT(iface->mask == 32,
	       "incorrect interface mask");
  UTEST_ASSERT(!net_iface_is_connected(iface),
	       "interface should not be connected");
  net_iface_destroy(&iface);
  UTEST_ASSERT(iface == NULL,
	       "destroyed interface should be NULL");
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_rtr ]---------------------------------------
static int test_net_iface_rtr()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  net_iface_t * iface= NULL;
  UTEST_ASSERT(net_iface_factory(node, net_iface_id_addr(IPV4(1,0,0,2)),
				 NET_IFACE_RTR, &iface) == ESUCCESS,
		"should be able to create rtr interface");
  UTEST_ASSERT(iface != NULL,
		"should return non-null interface");
  UTEST_ASSERT(iface->type == NET_IFACE_RTR,
		"interface type should be RTR");
  UTEST_ASSERT(iface->addr == IPV4(1,0,0,2),
		"incorrect interface address");
  UTEST_ASSERT(iface->mask == 32,
		"incorrect interface mask");
  UTEST_ASSERT(!net_iface_is_connected(iface),
		"interface should not be connected");
  net_iface_destroy(&iface);
  UTEST_ASSERT(iface == NULL,
		"destroyed interface should be NULL");
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_ptp ]---------------------------------------
static int test_net_iface_ptp()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  net_iface_t * iface= NULL;
  net_iface_id_t tIfaceID= net_iface_id_pfx(IPV4(192,168,0,1), 24);
  UTEST_ASSERT(net_iface_factory(node, tIfaceID, NET_IFACE_PTP,
				  &iface) == ESUCCESS,
		"should be able to create ptp interface");
  UTEST_ASSERT(iface != NULL,
		"should return non-null interface");
  UTEST_ASSERT(iface->type == NET_IFACE_PTP,
		"interface type should be PTP");
  UTEST_ASSERT(iface->addr == IPV4(192,168,0,1),
		"incorrect interface address");
  UTEST_ASSERT(iface->mask == 24,
		"incorrect interface mask");
  UTEST_ASSERT(!net_iface_is_connected(iface),
		"interface should not be connected");
  net_iface_destroy(&iface);
  UTEST_ASSERT(iface == NULL,
		"destroyed interface should be NULL");
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_ptmp ]--------------------------------------
static int test_net_iface_ptmp()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  net_iface_t * iface= NULL;
  net_iface_id_t iface_id= net_iface_id_pfx(IPV4(192,168,0,1), 24);
  ip_pfx_t pfx= IPV4PFX(192,168,0,0,24);
  UTEST_ASSERT(net_iface_factory(node, iface_id, NET_IFACE_PTMP,
				  &iface) == ESUCCESS,
		"should be able to create ptmp interface");
  UTEST_ASSERT(iface != NULL,
		"should return non-null interface");
  UTEST_ASSERT(iface->type == NET_IFACE_PTMP,
		"interface type should be PTMP");
  UTEST_ASSERT(iface->addr == IPV4(192,168,0,1),
		"incorrect interface address");
  UTEST_ASSERT(iface->mask == 24,
		"incorrect interface mask");
  UTEST_ASSERT(!net_iface_is_connected(iface),
		"interface should not be connected");
  UTEST_ASSERT(net_iface_has_prefix(iface, pfx),
		"interface should have prefix");
  net_iface_destroy(&iface);
  UTEST_ASSERT(iface == NULL,
		"destroyed interface should be NULL");
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_virtual ]-----------------------------------
static int test_net_iface_virtual()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  net_iface_t * iface= NULL;
  net_addr_t addr= IPV4(172,0,0,1);
  net_iface_id_t iface_id= net_iface_id_addr(addr);
  UTEST_ASSERT(net_iface_factory(node, iface_id,
				  NET_IFACE_VIRTUAL,
				  &iface) == ESUCCESS,
		"should be able to create virtual interface");
  UTEST_ASSERT(iface != NULL,
		"should return non-null interface");
  UTEST_ASSERT(iface->type == NET_IFACE_VIRTUAL,
		"interface type should be VIRTUAL");
  UTEST_ASSERT(iface->addr == addr,
		"incorrect interface address");
  UTEST_ASSERT(iface->mask == 32,
		"incorrect interface mask");
  UTEST_ASSERT(!net_iface_is_connected(iface),
		"interface should not be connected");
  net_iface_destroy(&iface);
  UTEST_ASSERT(iface == NULL,
		"destroyed interface should be NULL");
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_str2type ]----------------------------------
static int test_net_iface_str2type()
{
  net_iface_type_t type;
  UTEST_ASSERT((net_iface_str2type("loopback", &type) == ESUCCESS) &&
		(type == NET_IFACE_LOOPBACK),
		"bad conversion for loopback type");
  UTEST_ASSERT((net_iface_str2type("router-to-router", &type) == ESUCCESS) &&
		(type == NET_IFACE_RTR),
		"bad conversion for router-to-router type");
  UTEST_ASSERT((net_iface_str2type("point-to-point", &type) == ESUCCESS) &&
		(type == NET_IFACE_PTP),
		"bad conversion for point-to-point type");
  UTEST_ASSERT((net_iface_str2type("point-to-multipoint", &type)
		 == ESUCCESS) && (type == NET_IFACE_PTMP),
		"bad conversion for point-to-multipoint type");
  UTEST_ASSERT((net_iface_str2type("virtual", &type) == ESUCCESS) &&
		(type == NET_IFACE_VIRTUAL),
		"bad conversion for virtual type");
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_list ]--------------------------------------
static int test_net_iface_list()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  net_ifaces_t * ifaces= net_links_create();
  net_iface_t * iface1= NULL;
  net_iface_t * iface2= NULL;
  net_addr_t addr1= IPV4(1,0,0,2);
  net_addr_t addr2= IPV4(1,0,0,3);
  UTEST_ASSERT(net_iface_factory(node, net_iface_id_addr(addr1),
				  NET_IFACE_RTR, &iface1) == ESUCCESS,
		"should be able to create interface");
  UTEST_ASSERT(net_iface_factory(node, net_iface_id_addr(addr2),
				  NET_IFACE_RTR, &iface2) == ESUCCESS,
		"should be able to create interface");
  UTEST_ASSERT(net_links_add(ifaces, iface1) >= 0,
		"should be able to add interface");
  UTEST_ASSERT(net_links_add(ifaces, iface2) >= 0,
		"should be able to add interface");
  net_links_destroy(&ifaces);
  UTEST_ASSERT(ifaces == NULL,
		"destroyed interface list should be NULL");
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_net_iface_list_duplicate ]----------------------------
static int test_net_iface_list_duplicate()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  net_ifaces_t * ifaces= net_links_create();
  net_iface_t * iface1= NULL;
  net_iface_t * iface2= NULL;
  net_addr_t addr= IPV4(1,0,0,2);
  UTEST_ASSERT(net_iface_factory(node, net_iface_id_addr(addr),
				  NET_IFACE_RTR, &iface1) == ESUCCESS,
		"should be able to create interface");
  UTEST_ASSERT(net_iface_factory(node, net_iface_id_addr(addr),
				  NET_IFACE_RTR, &iface2) == ESUCCESS,
		"should be able to create interface");
  UTEST_ASSERT(net_links_add(ifaces, iface1) >= 0,
		"should be able to add interface");
  UTEST_ASSERT(net_links_add(ifaces, iface2) < 0,
		"should not be able to add interface with same ID");
  net_iface_destroy(&iface2);
  net_links_destroy(&ifaces);
  UTEST_ASSERT(ifaces == NULL,
	       "destroyed interface list should be NULL");
  node_destroy(&node);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET LINKS
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_link ]--------------------------------------------
static int test_net_link()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_iface_t * link= NULL;
  net_iface_id_t tIfaceID;
  UTEST_ASSERT((net_link_create_rtr(node1, node2, BIDIR, &link)
		 == ESUCCESS) &&
		(link != NULL),
		"link creation should succeed");
  tIfaceID= net_iface_id(link);
  UTEST_ASSERT((tIfaceID.network == link->addr) &&
		(tIfaceID.mask == link->mask),
		"link identifier is incorrect");
  UTEST_ASSERT(link->type == NET_IFACE_RTR,
		"link type is not correct");
  UTEST_ASSERT(link->owner == node1,
		"owner is not correct");
  UTEST_ASSERT((link->dest.iface != NULL) &&
		(link->dest.iface->owner == node2),
		"destination node is not correct");
  UTEST_ASSERT(net_iface_is_enabled(link) != 0,
		"link should be up");
  UTEST_ASSERT((net_iface_get_delay(link) == 0) &&
		(net_iface_get_capacity(link) == 0) &&
		(net_iface_get_load(link) == 0),
		"link attributes are not correct");
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_forward ]------------------------------------
static int test_net_link_forward()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_msg_t * msg= message_create(NET_ADDR_ANY, NET_ADDR_ANY, 0, 0,
				  (void *) 12345, NULL);
  net_iface_t * iface= NULL;
  simulator_t * sim= network_get_simulator(network_get_default());
  net_send_ctx_t * ctx;
  net_link_create_rtr(node1, node2, BIDIR, &iface);
  thread_set_simulator(sim);
  UTEST_ASSERT(net_iface_send(iface, NET_ADDR_ANY, msg) == ESUCCESS,
		"link forward should succeed");
  UTEST_ASSERT(sim_get_num_events(sim) == 1,
		"incorrect number of queued events");
  ctx= (net_send_ctx_t *) sim_get_event(sim, 0);
  UTEST_ASSERT(ctx->msg == msg,
		"incorrect queued message");
  UTEST_ASSERT(ctx->dst_iface == iface->dest.iface,
		"incorrect destination interface message");
  sim_clear(sim);
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_forward_down ]-------------------------------
static int test_net_link_forward_down()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_msg_t * msg= message_create(NET_ADDR_ANY, NET_ADDR_ANY, 0, 0,
				  (void *) 12345, NULL);
  net_iface_t * iface= NULL;
  simulator_t * sim= network_get_simulator(network_get_default());
  net_link_create_rtr(node1, node2, BIDIR, &iface);
  net_iface_set_enabled(iface, 0);
  thread_set_simulator(sim);
  UTEST_ASSERT(net_iface_send(iface, NET_ADDR_ANY, msg) == ENET_LINK_DOWN,
		"link forward should fail");
  UTEST_ASSERT(sim_get_num_events(sim) == 0,
		"incorrect number of queued events");
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptp ]----------------------------------------
static int test_net_link_ptp()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_addr_t addr1= IPV4(192,168,0,1);
  net_addr_t addr2= IPV4(192,168,0,2);
  net_iface_t * link= NULL;
  UTEST_ASSERT(net_link_create_ptp(node1, net_iface_id_pfx(addr1, 30),
				    node2, net_iface_id_pfx(addr2, 30),
				    BIDIR, &link)
		== ESUCCESS,
		"link creation should succeed");
  UTEST_ASSERT(link != NULL,
		"return link should not be NULL");
  UTEST_ASSERT(link->type == NET_IFACE_PTP,
		"interface type is incorrect");
  UTEST_ASSERT(link->owner == node1,
		"owner is not correct");
  UTEST_ASSERT(link->addr == addr1,
		"interface address is incorrect");
  UTEST_ASSERT(link->mask == 30,
		"interface mask is incorrect");
  UTEST_ASSERT((link->dest.iface != NULL) &&
		(link->dest.iface->owner == node2),
		"destination node is incorrect");
  UTEST_ASSERT((net_iface_get_delay(link) == 0) &&
		(net_iface_get_capacity(link) == 0) &&
		(net_iface_get_load(link) == 0),
		"link attributes are not correct");
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptp_forward ]--------------------------------
static int test_net_link_ptp_forward()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_addr_t addr1= IPV4(192,168,0,1);
  net_addr_t addr2= IPV4(192,168,0,2);
  net_iface_t * iface= NULL;
  net_msg_t * msg= message_create(NET_ADDR_ANY, NET_ADDR_ANY, 0, 0,
				   (void *) 12345, NULL);
  simulator_t * sim= network_get_simulator(network_get_default());
  net_send_ctx_t * ctx;
  net_link_create_ptp(node1, net_iface_id_pfx(addr1, 30),
		      node2, net_iface_id_pfx(addr2, 30),
		      BIDIR, &iface);
  thread_set_simulator(sim);
  UTEST_ASSERT(net_iface_send(iface, NET_ADDR_ANY, msg)
		== ESUCCESS,
		"link forward should succeed");
  UTEST_ASSERT(sim_get_num_events(sim) == 1,
		"incorrect number of queued events");
  ctx= (net_send_ctx_t *) sim_get_event(sim, 0);
  UTEST_ASSERT(ctx->msg == msg,
		"incorrect queued message");
  UTEST_ASSERT(ctx->dst_iface == iface->dest.iface,
		"incorrect destination interface message");
  sim_clear(sim);
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptp_forward_down ]---------------------------
static int test_net_link_ptp_forward_down()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_addr_t addr1= IPV4(192,168,0,1);
  net_addr_t addr2= IPV4(192,168,0,2);
  net_iface_t * iface= NULL;
  net_msg_t * msg= message_create(NET_ADDR_ANY, NET_ADDR_ANY, 0, 0,
				   (void *) 12345, NULL);
  simulator_t * sim= network_get_simulator(network_get_default());
  net_link_create_ptp(node1, net_iface_id_pfx(addr1, 30),
		      node2, net_iface_id_pfx(addr2, 30),
		      BIDIR, &iface);
  net_iface_set_enabled(iface, 0);
  thread_set_simulator(sim);
  UTEST_ASSERT(net_iface_send(iface, NET_ADDR_ANY, msg)
		== ENET_LINK_DOWN,
		"link forward should fail");
  UTEST_ASSERT(sim_get_num_events(sim) == 0,
		"incorrect number of queued events");
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptmp ]---------------------------------------
static int test_net_link_ptmp()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_subnet_t * subnet= subnet_create(IPV4(192,168,0,0), 16,
				       NET_SUBNET_TYPE_TRANSIT);
  net_iface_t * link= NULL;
  ip_pfx_t iface_id;
  UTEST_ASSERT((net_link_create_ptmp(node1, subnet,
				      IPV4(192,168,0,1),
				      &link) == ESUCCESS) &&
		(link != NULL),
		"link creation should succeed");
  iface_id= net_iface_id(link);
  UTEST_ASSERT((iface_id.network == link->addr) &&
		(iface_id.mask == link->mask),
		"link identifier is incorrect");
  UTEST_ASSERT(link->type == NET_IFACE_PTMP,
		"link type is not correct");
  UTEST_ASSERT(link->owner == node1,
		"owner is not correct");
  UTEST_ASSERT(link->dest.subnet == subnet,
		"destination subnet is not correct");
  UTEST_ASSERT(link->addr == IPV4(192,168,0,1),
		"interface address is not correct");
  UTEST_ASSERT(net_iface_is_enabled(link) != 0,
		"link should be up");
  UTEST_ASSERT((net_iface_get_delay(link) == 0) &&
		(net_iface_get_capacity(link) == 0) &&
		(net_iface_get_load(link) == 0),
		"link attributes are not correct");
  subnet_destroy(&subnet);
  UTEST_ASSERT(subnet == NULL,
		"destroyed subnet should be NULL");
  node_destroy(&node1);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptmp_dup ]-----------------------------------
static int test_net_link_ptmp_dup()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_subnet_t * subnet= subnet_create(IPV4(192,168,0,0), 16,
				       NET_SUBNET_TYPE_TRANSIT);
  UTEST_ASSERT((net_link_create_ptmp(node1, subnet,
				      IPV4(192,168,0,0),
				      NULL) == ESUCCESS),
		"link creation should succeed");
  UTEST_ASSERT((net_link_create_ptmp(node2, subnet,
				      IPV4(192,168,0,0),
				      NULL) == ENET_LINK_DUPLICATE),
		"link creation should fail (link-duplicate)");
  node_destroy(&node1);
  node_destroy(&node2);
  subnet_destroy(&subnet);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptmp_forward ]-------------------------------
static int test_net_link_ptmp_forward()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_subnet_t * subnet= subnet_create(IPV4(192,168,0,0), 16,
				       NET_SUBNET_TYPE_TRANSIT);
  net_iface_t * iface1= NULL;
  net_iface_t * iface2= NULL;
  net_msg_t * msg= message_create(NET_ADDR_ANY, NET_ADDR_ANY, 0, 0,
				  (void *) 12345, NULL);
  simulator_t * sim= network_get_simulator(network_get_default());
  net_send_ctx_t * ctx;
  net_link_create_ptmp(node1, subnet, IPV4(192,168,0,1), &iface1);
  net_link_create_ptmp(node2, subnet, IPV4(192,168,0,2), &iface2);
  thread_set_simulator(sim);
  UTEST_ASSERT(net_iface_send(iface1, IPV4(192,168,0,2), msg)
		== ESUCCESS,
		"link forward should succeed");
  UTEST_ASSERT(sim_get_num_events(sim) == 1,
		"incorrect number of queued events");
  ctx= (net_send_ctx_t *) sim_get_event(sim, 0);
  UTEST_ASSERT(ctx->msg == msg,
		"incorrect queued message");
  UTEST_ASSERT(ctx->dst_iface == iface2,
		"incorrect destination interface message");
  subnet_destroy(&subnet);
  sim_clear(sim);
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

// -----[ test_net_link_ptmp_forward_unreach ]-----------------------
static int test_net_link_ptmp_forward_unreach()
{
  net_node_t * node= __node_create(IPV4(1,0,0,0));
  net_subnet_t * subnet= subnet_create(IPV4(192,168,0,0), 16,
				       NET_SUBNET_TYPE_TRANSIT);
  net_iface_t * iface1= NULL;
  net_msg_t * msg= message_create(NET_ADDR_ANY, NET_ADDR_ANY, 0, 0,
				  (void *) 12345, NULL);
  simulator_t * sim= network_get_simulator(network_get_default());
  net_link_create_ptmp(node, subnet, IPV4(192,168,0,1), &iface1);

  thread_set_simulator(sim);
  UTEST_ASSERT(net_iface_send(iface1, IPV4(192,168,0,2), msg)
		== ENET_HOST_UNREACH,
		"link forward should fail (host-unreach)");
  UTEST_ASSERT(sim_get_num_events(sim) == 0,
		"incorrect number of queued events");
  subnet_destroy(&subnet);
  sim_clear(sim);
  subnet_destroy(&subnet);
  node_destroy(&node);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET ROUTING TABLE
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_rt_entries ]--------------------------------------
static int test_net_rt_entries()
{
  rt_entries_t * entries= rt_entries_create();
  net_iface_t * iface;
  rt_entry_t entry1;
  rt_entry_t entry2;
  rt_entry_t * entry;

  net_iface_factory(NULL, IPV4PFX(10,0,0,1,30), NET_IFACE_PTP, &iface);
  entry1.gateway= IPV4(1,0,0,0);
  entry1.oif= iface;
  entry2.gateway= IPV4(2,0,0,0);
  entry2.oif= iface;
  UTEST_ASSERT(entries != NULL, "RT entries creation should succeed");
  UTEST_ASSERT(rt_entries_add(entries, rt_entry_copy(&entry1)) == ESUCCESS,
		"RT entries addition should succeed");
  UTEST_ASSERT(rt_entries_size(entries) == 1, "RT entries size should be 1");
  UTEST_ASSERT(rt_entries_add(entries, rt_entry_copy(&entry2)) == ESUCCESS,
		"RT entries addition should succeed");
  UTEST_ASSERT(rt_entries_size(entries) == 2, "RT entries size should be 2");
  entry= rt_entry_copy(&entry1);
  UTEST_ASSERT(rt_entries_add(entries, entry) < 0,
		"RT entries duplicate addition should not succeed");
  rt_entry_destroy(&entry);
  UTEST_ASSERT(rt_entries_del(entries, &entry1) == ESUCCESS,
		"RT entries removal should succeed");
  UTEST_ASSERT(rt_entries_size(entries) == 1, "RT entries size should be 1");
  UTEST_ASSERT(rt_entries_del(entries, &entry1) != ESUCCESS,
		"RT entries removal of unexisting entry should not succeed");
  rt_entries_destroy(&entries);
  UTEST_ASSERT(entries == NULL, "destroyed RT entries should be NULL");
  net_iface_destroy(&iface);
  return UTEST_SUCCESS;
}

// -----[ test_net_rt ]----------------------------------------------
static int test_net_rt()
{
  net_rt_t * rt= rt_create();
  UTEST_ASSERT(rt != NULL, "RT creation should succeed");
  rt_destroy(&rt);
  UTEST_ASSERT(rt == NULL, "destroyed RT should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_rt_add ]------------------------------------------
static int test_net_rt_add()
{
  net_rt_t * rt= rt_create();
  net_iface_t * iface;
  ip_pfx_t pfx= IPV4PFX(192,168,1,0,24);
  rt_info_t * rtinfo;
  
  net_iface_factory(NULL, IPV4PFX(10,0,0,1,30), NET_IFACE_PTP, &iface);
  rtinfo= rt_info_create(pfx, 0, NET_ROUTE_STATIC);
  rt_info_add_entry(rtinfo, iface, NET_ADDR_ANY);
  UTEST_ASSERT(rtinfo != NULL, "route info creation should succeed");
  UTEST_ASSERT(rt_add_route(rt, pfx, rtinfo) == ESUCCESS,
		"route addition should succeed");
  rt_destroy(&rt);
  net_iface_destroy(&iface);
  return UTEST_SUCCESS;
}

// -----[ test_net_rt_add_dup ]--------------------------------------
static int test_net_rt_add_dup()
{
  net_rt_t * rt= rt_create();
  net_iface_t * iface;
  ip_pfx_t pfx= IPV4PFX(192,168,1,0,24);
  rt_info_t * rtinfo;

  net_iface_factory(NULL, IPV4PFX(10,0,0,1,30), NET_IFACE_PTP, &iface);
  rtinfo= rt_info_create(pfx, 0, NET_ROUTE_STATIC);
  rt_info_add_entry(rtinfo, iface, NET_ADDR_ANY);
  UTEST_ASSERT(rtinfo != NULL, "route info creation should succeed");
  UTEST_ASSERT(rt_add_route(rt, pfx, rtinfo) == ESUCCESS,
		"route addition should succeed");
  rtinfo= rt_info_create(pfx,0, NET_ROUTE_STATIC);
  rt_info_add_entry(rtinfo, iface, NET_ADDR_ANY);
  UTEST_ASSERT(rtinfo != NULL, "route info creation should succeed");
  UTEST_ASSERT(rt_add_route(rt, pfx, rtinfo) == ENET_RT_DUPLICATE,
		"route addition should fail (duplicate)");
  rt_info_destroy(&rtinfo);
  rt_destroy(&rt);
  net_iface_destroy(&iface);
  return UTEST_SUCCESS;
}

// -----[ test_net_rt_lookup ]---------------------------------------
static int test_net_rt_lookup()
{
  net_rt_t * rt= rt_create();
  net_iface_t * iface;
  ip_pfx_t pfx[3]= { IPV4PFX(192,168,1,0,24),
		     IPV4PFX(192,168,2,0,24),
		     IPV4PFX(192,168,2,128,25) };
  rt_info_t * rtinfo[3];
  int index;

  net_iface_factory(NULL, IPV4PFX(10,0,0,1,30), NET_IFACE_PTP, &iface);

  for (index= 0; index < 3; index++) {
    rtinfo[index]= rt_info_create(pfx[index], 0, NET_ROUTE_STATIC);
    rt_info_add_entry(rtinfo[index], iface, NET_ADDR_ANY);
    UTEST_ASSERT(rtinfo[index] != NULL, "route info creation should succeed");
    UTEST_ASSERT(rt_add_route(rt, pfx[index], rtinfo[index]) == ESUCCESS,
		  "route addition should succeed");
  }

  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,0,1), NET_ROUTE_ANY) == NULL,
		"should not return a result for 192.168.0.1");
  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,1,1), NET_ROUTE_ANY)
		== rtinfo[0], "should returned a result for 192.168.1.1");
  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,2,1), NET_ROUTE_ANY)
		== rtinfo[1], "should return a result for 192.168.2.1");
  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,2,129), NET_ROUTE_ANY)
		== rtinfo[2], "should return a result for 192.168.2.129");
  rt_destroy(&rt);
  net_iface_destroy(&iface);
  return UTEST_SUCCESS;
}


// -----[ test_net_rt_del ]------------------------------------------
static int test_net_rt_del()
{
  net_rt_t * rt= rt_create();
  net_iface_t * iface;
  ip_pfx_t pfx[2]= { IPV4PFX(192,168,1,0,24),
		     IPV4PFX(192,168,2,0,24) };
  rt_info_t * rtinfo[2];
  rt_filter_t * filter;
  int index;

  net_iface_factory(NULL, IPV4PFX(10,0,0,1,30), NET_IFACE_PTP, &iface);

  for (index= 0; index < 2; index++) {
    rtinfo[index]= rt_info_create(pfx[index], 0, NET_ROUTE_STATIC);
    rt_info_add_entry(rtinfo[index], iface, NET_ADDR_ANY);
    UTEST_ASSERT(rtinfo[index] != NULL, "route info creation should succeed");
    UTEST_ASSERT(rt_add_route(rt, pfx[index], rtinfo[index]) == ESUCCESS,
		"route addition should succeed");
  }

  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,1,1), NET_ROUTE_ANY)
		== rtinfo[0], "should return a result for 192.168.1.1");
  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,2,1), NET_ROUTE_ANY)
		== rtinfo[1], "should return a result for 192.168.2.1");

  filter= rt_filter_create();
  filter->prefix= &pfx[0];
  filter->type= NET_ROUTE_IGP;
  UTEST_ASSERT(rt_del_routes(rt, filter) == ESUCCESS,
		"route removal should succeed");
  UTEST_ASSERT(filter->matches == 0,
		"incorrect number of removal(s)");
  rt_filter_destroy(&filter);


  filter= rt_filter_create();
  filter->prefix= &pfx[0];
  UTEST_ASSERT(rt_del_routes(rt, filter) == ESUCCESS,
		"route removal should succeed");
  UTEST_ASSERT(filter->matches == 1,
		"incorrect number of removal(s)");
  rt_filter_destroy(&filter);

  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,1,1), NET_ROUTE_ANY)
		== NULL, "should not return a result for 192.168.1.1");
  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,2,1), NET_ROUTE_ANY)
		== rtinfo[1], "should return a result for 192.168.2.1");

  filter= rt_filter_create();
  filter->prefix= &pfx[1];
  UTEST_ASSERT(rt_del_routes(rt, filter) == ESUCCESS,
		"route removal should succeed");
  UTEST_ASSERT(filter->matches == 1,
		"incorrect number of removal(s)");
  rt_filter_destroy(&filter);

  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,1,1), NET_ROUTE_ANY)
		== NULL, "should not return a result for 192.168.1.1");
  UTEST_ASSERT(rt_find_best(rt, IPV4(192,168,2,1), NET_ROUTE_ANY)
		== NULL, "should not return a result for 192.168.2.1");

  rt_destroy(&rt);
  net_iface_destroy(&iface);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET TUNNELS
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_tunnel ]------------------------------------------
static int test_net_tunnel()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_iface_t * iface= NULL;
  net_iface_t * tunnel= NULL;
  net_link_create_rtr(node1, node2, BIDIR, &iface);
  UTEST_ASSERT((ipip_link_create(node1, IPV4(2,0,0,0),
				  IPV4(127,0,0,1),
				  NULL,
				  NET_ADDR_ANY,
				  &tunnel) == ESUCCESS) &&
		(tunnel != NULL),
		"ipip_link_create() should succeed");
  UTEST_ASSERT(tunnel->type == NET_IFACE_VIRTUAL,
		"link type is incorrect");
  UTEST_ASSERT(tunnel->owner == node1,
		"owner is not correct");
  UTEST_ASSERT(tunnel->dest.end_point == node2->rid,
		"tunnel endpoint is incorrect");
  UTEST_ASSERT(tunnel->addr == IPV4(127,0,0,1),
		"interface address is incorrect");
  UTEST_ASSERT(net_iface_is_enabled(tunnel) != 0,
		"link should be up");
  net_link_destroy(&tunnel);
  UTEST_ASSERT(tunnel == NULL,
		"destroyed link should be NULL");
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

// -----[ test_net_tunnel_forward ]----------------------------------
static int test_net_tunnel_forward()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_iface_t * iface= NULL;
  net_iface_t * tunnel= NULL;
  net_msg_t * msg= message_create(NET_ADDR_ANY, NET_ADDR_ANY, 0, 0,
				  (void *) 12345, NULL);
  simulator_t * sim= network_get_simulator(network_get_default());
  net_error_t error;
  net_send_ctx_t * ctx;

  net_link_create_rtr(node1, node2, BIDIR, &iface);
  node_rt_add_route_link(node1, IPV4PFX(2,0,0,0,32), iface,
			 NET_ADDR_ANY, 1, NET_ROUTE_STATIC);
  node_ipip_enable(node2);
  UTEST_ASSERT((ipip_link_create(node1, IPV4(2,0,0,0),
				  IPV4(2,0,0,0),
				  NULL,
				  NET_ADDR_ANY,
				  &tunnel) == ESUCCESS) &&
		(tunnel != NULL),
		"ipip_link_create() should succeed");
  thread_set_simulator(sim);
  UTEST_ASSERT((error= net_iface_send(tunnel, NET_ADDR_ANY, msg))
		== ESUCCESS,
		"tunnel forward should succeed (%s)",
		network_strerror(error));
  UTEST_ASSERT(sim_get_num_events(sim) == 1,
		"incorrect number of queued events");
  ctx= (net_send_ctx_t *) sim_get_event(sim, 0);
  UTEST_ASSERT(ctx->msg->protocol == NET_PROTOCOL_IPIP,
		"incorrect protocol for outer header");
  UTEST_ASSERT(ctx->msg->payload == msg,
		"incorrect payload for encapsulated packet");
  sim_step(sim, 1);
  net_link_destroy(&tunnel);
  UTEST_ASSERT(tunnel == NULL, "destroyed tunnel should be NULL");
  sim_clear(sim);
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

static int _msg_delivered;
static int _debug_proto_handler(net_msg_t * msg)
{
  _msg_delivered= 1;
  return ESUCCESS;
}

// -----[ test_net_tunnel_recv ]---------------------------
static int test_net_tunnel_recv()
{
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(2,0,0,0));
  net_iface_t * iface= NULL;
  net_msg_t * msg= message_create(NET_ADDR_ANY, IPV4(2,0,0,0),
				  NET_PROTOCOL_DEBUG, 255,
				  (void *) 12345, NULL);
  simulator_t * sim= network_get_simulator(network_get_default());
  net_error_t error;

  _msg_delivered= 0;

  net_link_create_rtr(node1, node2, BIDIR, &iface);
  error= node_add_tunnel(node1, IPV4(2,0,0,1), IPV4(1,0,0,1),
			 NULL, NET_ADDR_ANY);
  UTEST_ASSERT(error == ESUCCESS, "node_add_tunnel should succeed (%s)",
	       network_strerror(error));
  error= node_add_tunnel(node2, IPV4(1,0,0,1), IPV4(2,0,0,1),
			 NULL, NET_ADDR_ANY);
  UTEST_ASSERT(error == ESUCCESS, "node_add_tunnel should succeed (%s)",
	       network_strerror(error));
  error= node_rt_add_route(node1, IPV4PFX(2,0,0,0,32), IPV4PFX(1,0,0,1,32),
			   NET_ADDR_ANY, 1, NET_ROUTE_STATIC);
  UTEST_ASSERT(error == ESUCCESS, "node_rt_add_route should succeed (%s)",
	       network_strerror(error));
  error= node_rt_add_route(node1, IPV4PFX(2,0,0,1,32), IPV4PFX(2,0,0,0,32),
		    NET_ADDR_ANY, 1, NET_ROUTE_STATIC);
  UTEST_ASSERT(error == ESUCCESS, "node_rt_add_route should succeed (%s)",
	       network_strerror(error));
  error= node_register_protocol(node2, NET_PROTOCOL_DEBUG,
				_debug_proto_handler);
  UTEST_ASSERT(error == ESUCCESS, "node_register_protocol should succeed (%s)",
	       network_strerror(error));

  error= node_send(node1, msg, NULL, sim);
  UTEST_ASSERT(error == ESUCCESS, "node_send_msg should succeed");
  thread_set_simulator(sim);
  sim_run(sim);
  UTEST_ASSERT(_msg_delivered == 1, "message has not been delivered");
  node_destroy(&node1);
  node_destroy(&node2);
  return UTEST_SUCCESS;
}

// -----[ test_net_tunnel_src ]--------------------------------------
static int test_net_tunnel_src()
{
  return UTEST_SKIPPED;
}


/////////////////////////////////////////////////////////////////////
//
// NET NETWORK
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_network ]-----------------------------------------
static int test_net_network()
{
  network_t * network= network_create();
  UTEST_ASSERT(network != NULL, "network creation should succeed");
  network_destroy(&network);
  UTEST_ASSERT(network == NULL, "destroyed network should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_network_add_node ]--------------------------------
static int test_net_network_add_node()
{
  network_t * network= network_create();
  net_node_t * node= __node_create(IPV4(1,0,0,0));
  UTEST_ASSERT(network_add_node(network, node) == ESUCCESS,
		"node addition should succeed");
  network_destroy(&network);
  return UTEST_SUCCESS;
}

// -----[ test_net_network_add_node_dup ]----------------------------
static int test_net_network_add_node_dup()
{
  network_t * network= network_create();
  net_node_t * node1= __node_create(IPV4(1,0,0,0));
  net_node_t * node2= __node_create(IPV4(1,0,0,0));
  UTEST_ASSERT(network_add_node(network, node1) == ESUCCESS,
		"node addition should succeed");
  UTEST_ASSERT(network_add_node(network, node2)
		== ENET_NODE_DUPLICATE,
		"duplicate node addition should fail");
  node_destroy(&node2);
  network_destroy(&network);
  return UTEST_SUCCESS;
}

// -----[ test_net_network_add_subnet ]------------------------------
static int test_net_network_add_subnet()
{
  network_t * network= network_create();
  net_subnet_t * subnet= subnet_create(IPV4(192,168,0,0), 16,
				       NET_SUBNET_TYPE_STUB);
  UTEST_ASSERT(network_add_subnet(network, subnet) == ESUCCESS,
		"subnet addition should succeed");
  network_destroy(&network);
  return UTEST_SUCCESS;
}

// -----[ test_net_network_add_subnet_dup ]--------------------------
static int test_net_network_add_subnet_dup()
{
  network_t * network= network_create();
  net_subnet_t * subnet1= subnet_create(IPV4(192,168,0,0), 16,
					NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnet2= subnet_create(IPV4(192,168,0,0), 16,
					NET_SUBNET_TYPE_STUB);
  UTEST_ASSERT(network_add_subnet(network, subnet1) == ESUCCESS,
		"subnet addition should succeed");
  UTEST_ASSERT(network_add_subnet(network, subnet2)
		== ENET_SUBNET_DUPLICATE,
		"duplicate subnet addition should fail");
  subnet_destroy(&subnet2);
  network_destroy(&network);
  return UTEST_SUCCESS;
}

// -----[ test_net_network_node_send ]-------------------------------
static int test_net_network_node_send()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  net_msg_t * msg= message_create(NET_ADDR_ANY,
				  ez_topo_get_node(eztopo, 1)->rid,
				  NET_PROTOCOL_RAW, 255,
				  NULL, NULL);
  ez_topo_igp_compute(eztopo, 1);
  UTEST_ASSERT(node_send(ez_topo_get_node(eztopo, 0), msg, NULL,
			 network_get_simulator(eztopo->network)) == ESUCCESS,
	       "node_send() should succeed");
  sim_clear(network_get_simulator(eztopo->network)),
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_network_node_send_src ]---------------------------
static int test_net_network_node_send_src()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  net_msg_t * msg= message_create(ez_topo_get_node(eztopo, 0)->rid,
				  ez_topo_get_node(eztopo, 1)->rid,
				  NET_PROTOCOL_RAW, 255,
				  NULL, NULL);
  ez_topo_igp_compute(eztopo, 1);
  UTEST_ASSERT(node_send(ez_topo_get_node(eztopo, 0), msg, NULL,
			  network_get_simulator(eztopo->network)) == ESUCCESS,
		"node_send() should succeed");
  sim_clear(network_get_simulator(eztopo->network));
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;  
}

// -----[ test_net_network_node_send_src_invalid ]-------------------
static int test_net_network_node_send_src_invalid()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  net_msg_t * msg= message_create(IPV4(1,2,3,4),
	 			  ez_topo_get_node(eztopo, 1)->rid,
				  NET_PROTOCOL_RAW, 255,
				  NULL, NULL);
  ez_topo_igp_compute(eztopo, 1);
  UTEST_ASSERT(node_send(ez_topo_get_node(eztopo, 0), msg, NULL,
			  network_get_simulator(eztopo->network)) != ESUCCESS,
		"node_send() should fail");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET ROUTING STATIC
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_rt_static_add ]-----------------------------------
static int test_net_rt_static_add()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  ip_pfx_t pfx= { .network= IPV4(192,168,1,0), .mask=24 };
  ip_pfx_t iface_pfx= { .network= IPV4(10,0,0,1), .mask=30 };

  node_add_iface(node, iface_pfx, NET_IFACE_PTP);
  UTEST_ASSERT(node_rt_add_route(node, pfx, iface_pfx, NET_ADDR_ANY,
				  0, NET_ROUTE_STATIC) == ESUCCESS,
		"static route addition should succeed");
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_net_rt_static_add_dup ]-------------------------------
static int test_net_rt_static_add_dup()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  ip_pfx_t pfx= { .network= IPV4(192,168,1,0), .mask=24 };
  ip_pfx_t iface_pfx= { .network= IPV4(10,0,0,1), .mask=30 };
  net_error_t error;

  node_add_iface(node, iface_pfx, NET_IFACE_PTP);
  UTEST_ASSERT(node_rt_add_route(node, pfx, iface_pfx, NET_ADDR_ANY,
				  0, NET_ROUTE_STATIC) == ESUCCESS,
		"static route addition should succeed");
  UTEST_ASSERT((error= node_rt_add_route(node, pfx, iface_pfx, NET_ADDR_ANY,
					  0, NET_ROUTE_STATIC)) == ENET_RT_DUPLICATE,
		"static route addition should fail (duplicate) %d", error);
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_net_rt_static_remove ]--------------------------------
static int test_net_rt_static_remove()
{
  net_node_t * node= __node_create(IPV4(1,0,0,1));
  ip_pfx_t pfx[2]= { IPV4PFX(192,168,1,0,24),
		     IPV4PFX(192,168,2,0,24)};
  ip_pfx_t iface_pfx= IPV4PFX(10,0,0,1,30);
  int index;
  
  node_add_iface(node, iface_pfx, NET_IFACE_PTP);
  
  for (index= 0; index < 2; index++) {
    UTEST_ASSERT(node_rt_add_route(node, pfx[index], IPV4PFX(10,0,0,1,30),
				    NET_ADDR_ANY, 0, NET_ROUTE_STATIC)
		  == ESUCCESS,
		  "static route addition should succeed");
  }

  UTEST_ASSERT(node_rt_del_route(node, &pfx[0], NULL, NULL, NET_ROUTE_STATIC)
		== ESUCCESS, "route removal should succeed");
  UTEST_ASSERT(node_rt_del_route(node, &pfx[1], NULL, NULL, NET_ROUTE_STATIC)
		== ESUCCESS, "route removal should succeed");

  node_destroy(&node);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET ROUTING IGP
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_igp_domain ]--------------------------------------
static int test_net_igp_domain()
{
  igp_domain_t *domain= igp_domain_create(11537, IGP_DOMAIN_IGP);
  UTEST_ASSERT(domain != NULL,	"igp_domain_create() should succeed");
  igp_domain_destroy(&domain);
  UTEST_ASSERT(domain == NULL, "destroyed domain should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_domain_add ]----------------------------------
static int test_net_igp_domain_add()
{
  network_t * network= network_create();
  igp_domain_t *domain= igp_domain_create(11537, IGP_DOMAIN_IGP);
  UTEST_ASSERT(domain != NULL,	"igp_domain_create() should succeed");
  UTEST_ASSERT(network_add_igp_domain(network, domain) == ESUCCESS,
		"Should be able to add domain 1");
  UTEST_ASSERT(network_find_igp_domain(network, 11537) == domain,
		"IGP domain 11537 should exist");
  network_destroy(&network);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_domain_add_dup ]------------------------------
static int test_net_igp_domain_add_dup()
{
  network_t * network= network_create();
  igp_domain_t * domain= igp_domain_create(1, IGP_DOMAIN_IGP);
  UTEST_ASSERT(network_add_igp_domain(network, domain) == ESUCCESS,
		"Should be able to add domain 1");
  domain= igp_domain_create(1, IGP_DOMAIN_IGP);
  UTEST_ASSERT(network_add_igp_domain(network, domain)
		== ENET_IGP_DOMAIN_DUPLICATE,
		"Should not be able to add duplicate domain 1");
  igp_domain_destroy(&domain);
  network_destroy(&network);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute ]-------------------------------------
static int test_net_igp_compute()
{
  network_t * network= network_create();
  net_node_t * node1= __node_create(IPV4(1,0,0,1));
  net_node_t * node2= __node_create(IPV4(1,0,0,2));
  net_node_t * node3= __node_create(IPV4(1,0,0,3));
  net_addr_t addr12= IPV4(192,168,0,1);
  net_addr_t addr21= IPV4(192,168,0,2);
  net_addr_t addr23= IPV4(192,168,0,5);
  net_addr_t addr32= IPV4(192,168,0,6);
  igp_domain_t * domain= igp_domain_create(1, IGP_DOMAIN_IGP);
  net_iface_t * pLink;
  
  network_add_node(network, node1);
  network_add_node(network, node2);
  network_add_node(network, node3);
  network_add_igp_domain(network, domain);
  UTEST_ASSERT(net_link_create_ptp(node1,
				    net_iface_id_pfx(addr12, 30),
				    node2,
				    net_iface_id_pfx(addr21, 30),
				    BIDIR, &pLink)
		== ESUCCESS,
		"ptp-link creation should succeed");
  UTEST_ASSERT(net_iface_set_metric(pLink, 0, 1, 1) == ESUCCESS,
		"should be able to set interface metric");
  UTEST_ASSERT(net_link_create_ptp(node2,
				    net_iface_id_pfx(addr23, 30),
				    node3,
				    net_iface_id_pfx(addr32, 30),
				    BIDIR, &pLink)
		== ESUCCESS,
		"ptp-link creation should succeed");
  UTEST_ASSERT(net_iface_set_metric(pLink, 0, 10, 1) == ESUCCESS,
		"should be able to set interface metric");
  igp_domain_add_router(domain, node1);
  igp_domain_add_router(domain, node2);
  igp_domain_add_router(domain, node3);
  igp_domain_compute(domain, 0);
  network_destroy(&network);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_line_rtr ]----------------------------
static int test_net_igp_compute_line_rtr()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  igp_domain_t * domain= network_find_igp_domain(eztopo->network, 1);
  spt_t * spt;
  UTEST_ASSERT(spt_bfs(ez_topo_get_node(eztopo, 0), domain, &spt) == ESUCCESS,
		"SPT computation should succeed");
  UTEST_ASSERT(spt != NULL, "SPT should not be NULL");
  spt_vertex_t * vertex= spt_get_vertex(spt, IPV4PFX(0,0,0,1,32));
  UTEST_ASSERT(vertex != NULL, "SPT vertex should exist for 0.0.0.1/32");
  UTEST_ASSERT(vertex->weight == 0, "Cost should be 0 for 0.0.0.1/32");
  UTEST_ASSERT(spt_vertices_size(vertex->preds) == 0,
		"vertex for 0.0.0.1/32 should have 0 predecessor (root)");
  UTEST_ASSERT((vertex->elem.type == NODE) &&
		(vertex->elem.node == ez_topo_get_node(eztopo, 0)),
		"incorrect SPT root vertex");
  vertex= spt_get_vertex(spt, IPV4PFX(0,0,0,2,32));
  UTEST_ASSERT(vertex != NULL, "SPT vertex should exist for 0.0.0.2/32");
  UTEST_ASSERT(vertex->weight == 1, "Cost should be 1 for 0.0.0.1/32");
  UTEST_ASSERT(spt_vertices_size(vertex->preds) == 1,
		"vertex for 0.0.0.2/32 should have 1 predecessor");
  spt_vertex_t * pred= vertex->preds->data[0];
  UTEST_ASSERT((pred->elem.type == NODE) &&
		(pred->elem.node == ez_topo_get_node(eztopo, 0)),
		"plop");
  spt_destroy(&spt);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_link_down ]---------------------------
static int test_net_igp_compute_link_down()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  rt_info_t * rtinfo;
  net_iface_set_enabled(ez_topo_get_link(eztopo, 0), 0);
  ez_topo_igp_compute(eztopo, 1);
  rtinfo= rt_find_exact(ez_topo_get_node(eztopo, 0)->rt, IPV4PFX(0,0,0,2,32),
			NET_ROUTE_IGP);
  UTEST_ASSERT(rtinfo == NULL, "RT info should not exist for 0.0.0.2/32");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_link_weight_0 ]-----------------------
static int test_net_igp_compute_link_weight_0()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  rt_info_t * rtinfo;
  net_iface_set_metric(ez_topo_get_link(eztopo, 0), 0, 0, 0);
  ez_topo_igp_compute(eztopo, 1);
  rtinfo= rt_find_exact(ez_topo_get_node(eztopo, 0)->rt, IPV4PFX(0,0,0,2,32),
			NET_ROUTE_IGP);
  UTEST_ASSERT(rtinfo == NULL, "RT info should not exist for 0.0.0.2/32");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_link_weight_max ]---------------------
static int test_net_igp_compute_link_weight_max()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  rt_info_t * rtinfo;
  net_iface_set_metric(ez_topo_get_link(eztopo, 0), 0, IGP_MAX_WEIGHT, 0);
  ez_topo_igp_compute(eztopo, 1);
  rtinfo= rt_find_exact(ez_topo_get_node(eztopo, 0)->rt, IPV4PFX(0,0,0,2,32),
			NET_ROUTE_IGP);
  UTEST_ASSERT(rtinfo == NULL, "RT info should not exist for 0.0.0.2/32");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_link_domain_boundary ]-------------------
static int test_net_igp_compute_link_domain_boundary()
{
  ez_node_t nodes[]= {
    { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
    { .type=NODE, .domain=2, .options=EZ_NODE_OPT_NO_LOOPBACK },
  };
  ez_edge_t edges[]= {
    { .src= 0, .dst= 1, .weight=1 },
  };
  ez_topo_t * eztopo= ez_topo_builder(2, nodes, 1, edges);
  rt_info_t * rtinfo;
  ez_topo_igp_compute(eztopo, 1);
  rtinfo= rt_find_exact(ez_topo_get_node(eztopo, 0)->rt, IPV4PFX(0,0,0,2,32),
			NET_ROUTE_IGP);
  UTEST_ASSERT(rtinfo == NULL, "RT info should not exist for 0.0.0.2/32");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_line_ptp ]----------------------------
static int test_net_igp_compute_line_ptp()
{
  ez_topo_t * eztopo= _ez_topo_line_ptp();
  igp_domain_t * domain= network_find_igp_domain(eztopo->network, 1);
  spt_t * spt;
  UTEST_ASSERT(spt_bfs(ez_topo_get_node(eztopo, 0), domain, &spt) == ESUCCESS,
		"SPT computation should succeed");
  UTEST_ASSERT(spt != NULL, "SPT should not be NULL");
  spt_vertex_t * vertex= spt_get_vertex(spt, IPV4PFX(0,0,0,1,32));
  UTEST_ASSERT(vertex != NULL, "SPT vertex should exist for 0.0.0.1/32");
  UTEST_ASSERT(vertex->weight == 0, "Cost should be 0 for 0.0.0.1/32");
  UTEST_ASSERT(spt_vertices_size(vertex->preds) == 0,
		"vertex for 0.0.0.1/32 should have 0 predecessor (root)");
  vertex= spt_get_vertex(spt, IPV4PFX(0,0,0,2,32));
  UTEST_ASSERT(vertex != NULL, "SPT vertex should exist for 0.0.0.2/32");
  UTEST_ASSERT(vertex->weight == 1, "Cost should be 1 for 0.0.0.1/32");
  UTEST_ASSERT(spt_vertices_size(vertex->preds) == 1,
		"vertex for 0.0.0.2/32 should have 1 predecessor");
  spt_destroy(&spt);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_triangle_rtr ]------------------------
static int test_net_igp_compute_triangle_rtr()
{
  ez_topo_t * eztopo= _ez_topo_triangle_rtr();
  igp_domain_t * domain= network_find_igp_domain(eztopo->network, 1);
  spt_t * spt;
  UTEST_ASSERT(spt_bfs(ez_topo_get_node(eztopo, 0), domain, &spt) == ESUCCESS,
		"SPT computation should succeed");
  UTEST_ASSERT(spt != NULL, "SPT should not be NULL");
  spt_vertex_t * info= spt_get_vertex(spt, IPV4PFX(0,0,0,1,32));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 0.0.0.1/32");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 1,
    "SPT info should contain a single entry for 0.0.0.1/32");*/
  info= spt_get_vertex(spt, IPV4PFX(0,0,0,2,32));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 0.0.0.2/32");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 1,
    "SPT info should contain a single entry for 0.0.0.2/32");*/
  info= spt_get_vertex(spt, IPV4PFX(0,0,0,3,32));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 0.0.0.3/32");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 1,
    "SPT info should contain a single entry for 0.0.0.3/32");*/
  spt_destroy(&spt);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_triangle_ptp ]------------------------
static int test_net_igp_compute_triangle_ptp()
{
  ez_topo_t * eztopo= _ez_topo_triangle_ptp();

  net_export_stream(gdsout, eztopo->network, NET_EXPORT_FORMAT_DOT);

  net_iface_set_metric(ez_topo_get_link(eztopo,0)->dest.iface,
		       0, 5, 0);
  igp_domain_t * domain= network_find_igp_domain(eztopo->network, 1);
  spt_t * spt;
  spt_bfs(ez_topo_get_node(eztopo, 0), domain, &spt);
  spt_to_graphviz(gdsout, spt);
  rt_info_t * rtinfo;
  igp_domain_compute(domain, 0);
  //ip_dest_t dest= { .type=NET_DEST_ANY };
  //rt_dump(gdsout, ez_topo_get_node(eztopo, 0)->rt, dest);
  rtinfo= rt_find_exact(ez_topo_get_node(eztopo, 0)->rt,
			IPV4PFX(192,168,0,0,30), NET_ROUTE_IGP);
  UTEST_ASSERT(rtinfo != NULL, "RT info for 192.168.0.0/30 should exist");
  UTEST_ASSERT(rtinfo->metric == 10, "IGP cost should be 10");
  ez_topo_destroy(&eztopo);
  return UTEST_SKIPPED;
}

// -----[ test_net_igp_compute_subnet ]------------------------------
static int test_net_igp_compute_subnet()
{
  ez_node_t nodes[]= {
    { .type=NODE, .domain=1 },
    { .type=NODE, .domain=1 },
    { .type=NODE, .domain=1 },
    { .type=SUBNET, .id.pfx=IPV4PFX(192,168,0,0,24) },
  };
  ez_edge_t edges[]= {
    { .src=0, .dst=3, .weight=1, .src_addr=IPV4(192,168,0,1) },
    { .src=1, .dst=3, .weight=1, .src_addr=IPV4(192,168,0,2) },
    { .src=2, .dst=3, .weight=1, .src_addr=IPV4(192,168,0,3) },
  };
  ez_topo_t * eztopo= ez_topo_builder(4, nodes, 3, edges);

  net_export_stream(gdsout, eztopo->network, NET_EXPORT_FORMAT_DOT);

  igp_domain_t * domain= network_find_igp_domain(eztopo->network, 1);
  spt_t * spt;
  UTEST_ASSERT(spt_bfs(ez_topo_get_node(eztopo, 0), domain, &spt) == ESUCCESS,
		"SPT computation should succeed");
  spt_to_graphviz(gdsout, spt);
  UTEST_ASSERT(spt != NULL, "SPT should not be NULL");
  spt_vertex_t * info= spt_get_vertex(spt, IPV4PFX(0,0,0,1,32));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 0.0.0.1/32");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 1,
    "SPT info should contain a single entry for 0.0.0.1/32");*/
  info= spt_get_vertex(spt, IPV4PFX(192,168,0,0,24));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 192.168.0.0/24");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 1,
    "SPT info should contain a single entry for 192.168.0.0/24");*/
  info= spt_get_vertex(spt, IPV4PFX(0,0,0,2,32));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 0.0.0.2/32");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 1,
    "SPT info should contain a single entry for 0.0.0.2/32");*/
  info= spt_get_vertex(spt, IPV4PFX(0,0,0,3,32));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 0.0.0.3/32");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 1,
    "SPT info should contain a single entry for 0.0.0.3/32");*/
  spt_destroy(&spt);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_subnet_stub ]-------------------------
static int test_net_igp_compute_subnet_stub()
{
  ez_node_t nodes[]= {
    { .type=NODE, .domain=1 },
    { .type=NODE, .domain=1 },
    { .type=NODE, .domain=1 },
    { .type=SUBNET, .id.pfx=IPV4PFX(192,168,0,0,24),
      .options=EZ_NODE_OPT_STUB },
  };
  ez_edge_t edges[]= {
    { .src=0, .dst=3, .weight=1, .src_addr=IPV4(192,168,0,1) },
    { .src=1, .dst=3, .weight=1, .src_addr=IPV4(192,168,0,2) },
    { .src=2, .dst=3, .weight=1, .src_addr=IPV4(192,168,0,3) },
  };
  ez_topo_t * eztopo= ez_topo_builder(4, nodes, 3, edges);
  igp_domain_t * domain= network_find_igp_domain(eztopo->network, 1);
  spt_t * spt;
  UTEST_ASSERT(spt_bfs(ez_topo_get_node(eztopo, 0), domain, &spt) == ESUCCESS,
		"SPT computation should succeed");
  UTEST_ASSERT(spt != NULL, "SPT should not be NULL");
  spt_vertex_t * info= spt_get_vertex(spt, IPV4PFX(0,0,0,1,32));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 0.0.0.1/32");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 1,
    "SPT info should contain a single entry for 0.0.0.1/32");*/
  info= spt_get_vertex(spt, IPV4PFX(192,168,0,0,24));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 192.168.0.0/24");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 1,
    "SPT info should contain a single entry for 192.168.0.0/24");*/
  info= spt_get_vertex(spt, IPV4PFX(0,0,0,2,32));
  UTEST_ASSERT(info == NULL, "no SPT info should exist for 0.0.0.2/32");
  info= spt_get_vertex(spt, IPV4PFX(0,0,0,3,32));
  UTEST_ASSERT(info == NULL, "no SPT info should exist for 0.0.0.3/32");
  spt_destroy(&spt);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_loopback ]----------------------------
static int test_net_igp_compute_loopback()
{
  ez_topo_t * eztopo= _ez_topo_triangle_ptp();
  UTEST_ASSERT(node_add_iface(ez_topo_get_node(eztopo, 0),
			       net_iface_id_addr(IPV4(1,0,0,0)),
			       NET_IFACE_LOOPBACK) == ESUCCESS,
		"creation of loopback should succeed");
  UTEST_ASSERT(node_add_iface(ez_topo_get_node(eztopo, 1),
			       net_iface_id_addr(IPV4(2,0,0,0)),
			       NET_IFACE_LOOPBACK) == ESUCCESS,
		"creation of loopback should succeed");
  UTEST_ASSERT(node_add_iface(ez_topo_get_node(eztopo, 2),
			       net_iface_id_addr(IPV4(3,0,0,0)),
			       NET_IFACE_LOOPBACK) == ESUCCESS,
		"creation of loopback should succeed");
  igp_domain_t * domain= network_find_igp_domain(eztopo->network, 1);
  igp_domain_compute(domain, 0);
  UTEST_ASSERT(rt_find_exact(ez_topo_get_node(eztopo, 0)->rt,
			      IPV4PFX(2,0,0,0,32), NET_ROUTE_IGP) != NULL,
		"RT entry for 2.0.0.0/32 should exist in 0.0.0.1");
  UTEST_ASSERT(rt_find_exact(ez_topo_get_node(eztopo, 0)->rt,
			      IPV4PFX(3,0,0,0,32), NET_ROUTE_IGP) != NULL,
		"RT entry for 3.0.0.0/32 should exist in 0.0.0.1");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_ecmp_square ]-------------------------
static int test_net_igp_compute_ecmp_square()
{
  ez_topo_t * eztopo= _ez_topo_square();
  igp_domain_t * domain;
  spt_t * spt;
  spt_vertex_t * info;
  domain= network_find_igp_domain(eztopo->network, 1);
  UTEST_ASSERT(spt_bfs(ez_topo_get_node(eztopo, 0), domain, &spt) == ESUCCESS,
		"SPT computation should succeed");
  info= spt_get_vertex(spt, IPV4PFX(0,0,0,4,32));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 0.0.0.4/32");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 2,
    "SPT info should contain 2 entries for 0.0.0.4/32");*/
  spt_destroy(&spt);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_compute_ecmp_complex ]------------------------
static int test_net_igp_compute_ecmp_complex()
{
  ez_topo_t * eztopo= _ez_topo_glasses();
  igp_domain_t * domain;
  spt_t * spt;
  spt_vertex_t * info;
  domain= network_find_igp_domain(eztopo->network, 1);
  UTEST_ASSERT(spt_bfs(ez_topo_get_node(eztopo, 0), domain, &spt) == ESUCCESS,
		"SPT computation should succeed");
  UTEST_ASSERT(spt != NULL, "SPT computation should succeed");
  info= spt_get_vertex(spt, IPV4PFX(0,0,0,7,32));
  UTEST_ASSERT(info != NULL, "SPT info should exist for 0.0.0.7/32");
  UTEST_ASSERT(info->weight == 6, "Cost should be 6 for 0.0.0.7/32");
  /*UTEST_ASSERT(rt_entries_size(info->entries) == 2,
    "SPT info should contain 2 entries for 0.0.0.7/32");*/
  spt_destroy(&spt);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_igp_ecmp3 ]---------------------------------------
static int test_net_igp_ecmp3()
{
  ez_node_t nodes[]= {
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
    { .type= NODE, .domain=1 },
  };
  ez_edge_t edges[]= {
    { .src=0, .dst=1, .weight=1 },
    { .src=1, .dst=2, .weight=1 },
    { .src=1, .dst=3, .weight=1 },
    { .src=2, .dst=4, .weight=1 },
    { .src=3, .dst=4, .weight=1 },
    { .src=3, .dst=6, .weight=2 },
    { .src=4, .dst=5, .weight=1 },
    { .src=4, .dst=6, .weight=1 },
    { .src=5, .dst=7, .weight=1 },
    { .src=6, .dst=7, .weight=1 },
    { .src=7, .dst=8, .weight=1 },
  };
  ez_topo_t * eztopo= ez_topo_builder(9, nodes, 11, edges);
  igp_domain_t * domain= network_find_igp_domain(eztopo->network, 1);
  spt_t * spt;
  UTEST_ASSERT(spt_bfs(ez_topo_get_node(eztopo, 1), domain, &spt) == ESUCCESS,
		"SPT computation should succeed");
  spt_to_graphviz(gdsout, spt);
  UTEST_ASSERT(spt != NULL, "SPT computation should succeed");
  spt_destroy(&spt);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// NET TRACES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_net_traces_ping ]-------------------------------------
static int test_net_traces_ping()
{
  ez_topo_t * eztopo= _ez_topo_triangle_rtr();
  net_node_t * node= ez_topo_get_node(eztopo, 0);
  ez_topo_igp_compute(eztopo, 1);
  UTEST_ASSERT(icmp_ping_send_recv(node, NET_ADDR_ANY,
				    IPV4(0,0,0,2), 255) == ESUCCESS,
		"ping from 0.0.0.1 to 0.0.0.2 should succeed");
  UTEST_ASSERT(icmp_ping_send_recv(node, NET_ADDR_ANY,
				    IPV4(0,0,0,3), 255) == ESUCCESS,
		"ping from 0.0.0.1 to 0.0.0.3 should succeed");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_ping_self_loopback ]-----------------------
static int test_net_traces_ping_self_loopback()
{
  network_t * network= network_create();
  net_node_t * node;
  node_create(IPV4(1,0,0,0), &node, NODE_OPTIONS_LOOPBACK);
  network_add_node(network, node);
  UTEST_ASSERT(icmp_ping_send_recv(node, NET_ADDR_ANY,
				    node->rid, 255) == ESUCCESS,
		"ping from 1.0.0.0 to 1.0.0.0 should succeed");
  network_destroy(&network);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_ping_self_iface ]--------------------------
static int test_net_traces_ping_self_iface()
{
  ez_topo_t * eztopo= _ez_topo_line_ptp();
  UTEST_ASSERT(icmp_ping_send_recv(ez_topo_get_node(eztopo, 0), NET_ADDR_ANY,
				    IPV4(192,168,0,1), 255) == ESUCCESS,
		"ping from 0.0.0.1 to 192.168.0.1 should succeed");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_ping_unreach ]-----------------------------
static int test_net_traces_ping_unreach()
{
  network_t * network= network_create();
  net_node_t * node;
  node_create(IPV4(1,0,0,0), &node, NODE_OPTIONS_LOOPBACK);
  network_add_node(network, node);
  UTEST_ASSERT(icmp_ping_send_recv(node, NET_ADDR_ANY,
				    IPV4(2,0,0,0), 255) == ENET_HOST_UNREACH,
		"ping from 1.0.0.0 to 2.0.0.0 should fail (host-unreach)");
  network_destroy(&network);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_ping_no_reply ]----------------------------
static int test_net_traces_ping_no_reply()
{
  ez_topo_t * eztopo= _ez_topo_line_ptp();
  ez_topo_igp_compute(eztopo, 1);
  net_iface_set_enabled(ez_topo_get_link(eztopo, 0)->dest.iface, 0);
  UTEST_ASSERT(icmp_ping_send_recv(ez_topo_get_node(eztopo, 0), NET_ADDR_ANY,
				    IPV4(192,168,0,2), 255) == ENET_NO_REPLY,
		"ping from 0.0.0.1 to 192.168.0.2 should fail (no reply)");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute ]------------------------------
static int test_net_traces_recordroute()
{
  ez_topo_t * eztopo= _ez_topo_triangle_rtr();
  ip_trace_t * trace= NULL;
  array_t * traces;
  ez_topo_igp_compute(eztopo, 1);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
				ez_topo_get_node(eztopo, 1)->rid,
			   255, NULL);
  UTEST_ASSERT(traces != NULL,
		"icmp_trace_send should not return NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "should contain a single trace")
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
	       "trace's status should be success");
  UTEST_ASSERT(ip_trace_length(trace) == 3,
	       "record-route trace's length should be 3 (%d)",
	       ip_trace_length(trace));
  UTEST_ASSERT((ip_trace_item_at(trace, 0)->elt.node ==
		ez_topo_get_node(eztopo, 0)) &&
	       (ip_trace_item_at(trace, 1)->elt.node ==
		ez_topo_get_node(eztopo, 2)) &&
	       (ip_trace_item_at(trace, 2)->elt.node ==
		ez_topo_get_node(eztopo, 1)),
	       "record-route trace's content is not correct");
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_unreach ]----------------------
static int test_net_traces_recordroute_unreach()
{
  ez_topo_t * eztopo= _ez_topo_triangle_ptp();
  ip_trace_t * trace= NULL;
  array_t * traces;

  ez_topo_igp_compute(eztopo, 1);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   IPV4(1,0,0,4),
			   255, NULL);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "there should be a unique trace");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ENET_HOST_UNREACH,
	       "trace's status should be host-unreach");
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_broken ]-----------------------
static int test_net_traces_recordroute_broken()
{
  return UTEST_SKIPPED;
}

// -----[ test_net_traces_recordroute_load ]-------------------------
static int test_net_traces_recordroute_load()
{
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  array_t * traces;
  ez_topo_t * eztopo= _ez_topo_triangle_rtr();
  ez_topo_igp_compute(eztopo, 1);
  opts= ip_options_create();
  ip_options_load(opts, 1000);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   ez_topo_get_node(eztopo, 1)->rid,
			   255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "there should be a unique trace");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success");
  UTEST_ASSERT(net_iface_get_load(ez_topo_get_link(eztopo, 0)) == 0,
		"load of link 0 should be 0");
  UTEST_ASSERT(net_iface_get_load(ez_topo_get_link(eztopo, 1)) == 1000,
		"load of link 1 should be 1000");
  UTEST_ASSERT(net_iface_get_load(ez_topo_get_link(eztopo, 2)->dest.iface) == 1000,
		"load of link 2 should be 1000");
  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_prefix ]-----------------------
static int test_net_traces_recordroute_prefix()
{
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  ez_topo_t * eztopo= _ez_topo_triangle_rtr();
  net_subnet_t * subnet= subnet_create(IPV4(255,0,0,0), 8,
				       NET_SUBNET_TYPE_STUB);
  net_iface_t * link;
  array_t * traces;
  network_add_subnet(eztopo->network, subnet);
  net_link_create_ptmp(ez_topo_get_node(eztopo, 1), subnet,
		       IPV4(255,0,0,1), &link);
  net_iface_set_metric(link, 0, 1, 0);
  ez_topo_igp_compute(eztopo, 1);
  opts= ip_options_create();
  ip_options_alt_dest(opts, IPV4PFX(255,0,0,0,8));
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   NET_ADDR_ANY, 255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "traces should contain a unique trace");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success");
  // Trace should contain 4 elements: 3 hops + 1 subnet
  // (0.0.0.1 0.0.0.3 0.0.0.2 255.0.0.0/8)
  UTEST_ASSERT(ip_trace_length(trace) == 4, "trace should contain 4 elements");
  _array_destroy(&traces);
  ip_options_destroy(&opts);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_prefix_unreach ]---------------
static int test_net_traces_recordroute_prefix_unreach()
{
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  ez_topo_t * eztopo= _ez_topo_triangle_rtr();
  array_t * traces;
  ez_topo_igp_compute(eztopo, 1);
  opts= ip_options_create();
  ip_options_alt_dest(opts, IPV4PFX(255,0,0,0,8));
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   NET_ADDR_ANY, 255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "traces should contain a unique trace");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ENET_NET_UNREACH,
		"trace's status should be net-unreach");
  // Trace should contain 1 element: 1 hop (0.0.0.1)
  UTEST_ASSERT(ip_trace_length(trace) == 1,
	       "trace should contain 1 element");
  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_prefix_load ]------------------
static int test_net_traces_recordroute_prefix_load()
{
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  ez_topo_t * eztopo= _ez_topo_triangle_rtr();
  net_subnet_t * subnet= subnet_create(IPV4(255,0,0,0), 8,
				       NET_SUBNET_TYPE_STUB);
  net_iface_t * link;
  array_t * traces;
  network_add_subnet(eztopo->network, subnet);
  net_link_create_ptmp(ez_topo_get_node(eztopo, 2), subnet,
		       IPV4(255,0,0,1), &link);
  net_iface_set_metric(link, 0, 1, 0);
  ez_topo_igp_compute(eztopo, 1);
  opts= ip_options_create();
  ip_options_alt_dest(opts, IPV4PFX(255,0,0,0,8));
  ip_options_load(opts, 1000);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   NET_ADDR_ANY, 255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "traces should contain a unique trace");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success");
  UTEST_ASSERT(net_iface_get_load(link) == 1000,
		"last mile's load should be 1000");
  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_qos ]--------------------------
static int test_net_traces_recordroute_qos()
{
  ez_topo_t * eztopo= _ez_topo_triangle_rtr();
  ip_trace_t * trace= NULL;
  ip_opt_t * opts;
  array_t * traces;

  ez_topo_igp_compute(eztopo, 1);
  opts= ip_options_create();
  ip_options_set(opts, IP_OPT_CAPACITY);
  ip_options_set(opts, IP_OPT_DELAY);
  ip_options_set(opts, IP_OPT_WEIGHT);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   ez_topo_get_node(eztopo, 1)->rid,
			   255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "traces should contain a unique trace");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success");
  UTEST_ASSERT(ip_trace_length(trace) == 3,
		"record-route trace's length should be 3 (%d)",
		ip_trace_length(trace));
  UTEST_ASSERT((ip_trace_item_at(trace, 0)->elt.node ==
		 ez_topo_get_node(eztopo, 0)) &&
		(ip_trace_item_at(trace, 1)->elt.node ==
		 ez_topo_get_node(eztopo, 2)) &&
		(ip_trace_item_at(trace, 2)->elt.node ==
		 ez_topo_get_node(eztopo, 1)),
		"record-route trace's content is not correct");
  UTEST_ASSERT(trace->weight == 2,
		"record-route trace's IGP weight is not correct");
  UTEST_ASSERT(trace->delay == 39,
		"record-route trace's delay is not correct (%d)",
		trace->delay);
  UTEST_ASSERT(trace->capacity == 512,
		"record-route trace's max capacity is not correct");
  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;  
}

// -----[ test_net_traces_recordroute_ecmp ]-------------------------
static int test_net_traces_recordroute_ecmp()
{
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  ez_topo_t * eztopo= _ez_topo_square();
  array_t * traces;

  ez_topo_igp_compute(eztopo, 1);
  opts= ip_options_create();
  ip_options_set(opts, IP_OPT_ECMP);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   ez_topo_get_node(eztopo, 3)->rid,
			   255, opts);
  UTEST_ASSERT(traces != NULL, "icmp_trace_send() should succeed");
  UTEST_ASSERT(_array_length(traces) == 2, "there should be 2 paths");

  // Check first trace (0.0.0.1 0.0.0.2 0.0.0.4)
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success");
  UTEST_ASSERT(ip_trace_length(trace) == 3,
	       "record-route trace's length should be 3 (%d)",
	       ip_trace_length(trace));
  UTEST_ASSERT((ip_trace_item_at(trace, 0)->elt.node ==
		ez_topo_get_node(eztopo, 0)) &&
	       (ip_trace_item_at(trace, 1)->elt.node ==
		ez_topo_get_node(eztopo, 1)) &&
	       (ip_trace_item_at(trace, 2)->elt.node ==
		ez_topo_get_node(eztopo, 3)),
	       "record-route trace's content is not correct");

  // Check first trace (0.0.0.1 0.0.0.3 0.0.0.4)
  _array_get_at(traces, 1, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success");
  UTEST_ASSERT(ip_trace_length(trace) == 3,
	       "record-route trace's length should be 3 (%d)",
	       ip_trace_length(trace));
  UTEST_ASSERT((ip_trace_item_at(trace, 0)->elt.node ==
		ez_topo_get_node(eztopo, 0)) &&
	       (ip_trace_item_at(trace, 1)->elt.node ==
		ez_topo_get_node(eztopo, 2)) &&
	       (ip_trace_item_at(trace, 2)->elt.node ==
		ez_topo_get_node(eztopo, 3)),
	       "record-route trace's content is not correct");

  _array_destroy(&traces);
  ip_options_destroy(&opts);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_ecmp_unreach ]-----------------
static int test_net_traces_recordroute_ecmp_unreach()
{
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  array_t * traces;
  ez_topo_t * eztopo= _ez_topo_square();
  ez_topo_igp_compute(eztopo, 1);
  net_iface_set_enabled(ez_topo_get_link(eztopo, 2), 0);
  opts= ip_options_create();
  ip_options_set(opts, IP_OPT_ECMP);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
				ez_topo_get_node(eztopo, 3)->rid,
			   255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 2, "there should be 2 traces");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ENET_LINK_DOWN,
	       "1st trace's status should be link-down (%s)",
	       network_strerror(trace->status));
  _array_get_at(traces, 1, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
	       "2nd trace's status should be success (%s)",
	       network_strerror(trace->status));
  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_ecmp_broken ]------------------
static int test_net_traces_recordroute_ecmp_broken()
{
  return UTEST_SKIPPED;
}

// -----[ test_net_traces_recordroute_ecmp_load ]--------------------
static int test_net_traces_recordroute_ecmp_load()
{
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  array_t * traces;
  ez_topo_t * eztopo= _ez_topo_square();
  ez_topo_igp_compute(eztopo, 1);
  opts= ip_options_create();
  ip_options_set(opts, IP_OPT_ECMP);
  ip_options_load(opts, 1000);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   ez_topo_get_node(eztopo, 3)->rid,
			   255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 2,
	       "there should be 2 traces");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success");
  _array_get_at(traces, 1, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success");
  UTEST_ASSERT(net_iface_get_load(ez_topo_get_link(eztopo, 0)) == 500,
		"load of link 0 should be 500");
  UTEST_ASSERT(net_iface_get_load(ez_topo_get_link(eztopo, 1)) == 500,
		"load of link 1 should be 500");
  UTEST_ASSERT(net_iface_get_load(ez_topo_get_link(eztopo, 2)) == 500,
		"load of link 2 should be 500");
  UTEST_ASSERT(net_iface_get_load(ez_topo_get_link(eztopo, 3)) == 500,
		"load of link 3 should be 500");
  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_tunnel ]-----------------------
static int test_net_traces_recordroute_tunnel()
{
  ez_topo_t * eztopo= _ez_topo_line_ptp();
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  array_t * traces;
  node_add_tunnel(ez_topo_get_node(eztopo, 0), IPV4(255,0,0,2),
		  IPV4(255,0,0,1), NULL, NET_ADDR_ANY);
  node_add_tunnel(ez_topo_get_node(eztopo, 1), IPV4(255,0,0,1),
		  IPV4(255,0,0,2), NULL, NET_ADDR_ANY);
  ez_topo_igp_compute(eztopo, 1);
  node_add_iface(ez_topo_get_node(eztopo, 1), IPV4PFX(255,0,0,255,32),
		 NET_IFACE_LOOPBACK);
  node_rt_add_route(ez_topo_get_node(eztopo, 0), IPV4PFX(255,0,0,255,32),
		    IPV4PFX(255,0,0,1,32), NET_ADDR_ANY, 0, NET_ROUTE_STATIC);
  opts= ip_options_create();
  ip_options_set(opts, IP_OPT_TUNNEL);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   IPV4(255,0,0,255),
			   255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "there should be a unique trace");

  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success");
  UTEST_ASSERT(ip_trace_length(trace) == 3,
		"trace's length should be 3");
  UTEST_ASSERT(ip_trace_item_at(trace, 0)->elt.node ==
		ez_topo_get_node(eztopo, 0),
		"first hop should be 0.0.0.1");
  UTEST_ASSERT(ip_trace_item_at(trace, 1)->elt.type == TRACE,
		"mid hop should be a tunnel trace");
  UTEST_ASSERT(ip_trace_item_at(trace, 2)->elt.node ==
		ez_topo_get_node(eztopo, 1),
		"last hop should be 0.0.0.2");

  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_tunnel_ecmp ]------------------
static int test_net_traces_recordroute_tunnel_ecmp()
{
  ez_topo_t * eztopo= _ez_topo_square();
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  array_t * traces;

  node_add_tunnel(ez_topo_get_node(eztopo, 0), IPV4(255,0,0,3),
		  IPV4(255,0,0,1), NULL, NET_ADDR_ANY);
  node_add_tunnel(ez_topo_get_node(eztopo, 3), IPV4(255,0,0,1),
		  IPV4(255,0,0,3), NULL, NET_ADDR_ANY);
  ez_topo_igp_compute(eztopo, 1);
  node_add_iface(ez_topo_get_node(eztopo, 3), IPV4PFX(255,0,0,255,32),
		 NET_IFACE_LOOPBACK);
  node_rt_add_route(ez_topo_get_node(eztopo, 0), IPV4PFX(255,0,0,255,32),
		    IPV4PFX(255,0,0,1,32), NET_ADDR_ANY, 0, NET_ROUTE_STATIC);
  opts= ip_options_create();
  ip_options_set(opts, IP_OPT_ECMP);
  ip_options_set(opts, IP_OPT_TUNNEL);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   IPV4(255,0,0,255), 255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 2, "there should be 2 traces");

  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"1st trace's status should be success");
  // Trace should contain 3 elements: 2 nodes, 1 trace
  // (0.0.0.1 [0.0.0.1 0.0.0.2 0.0.0.4] 0.0.0.4)
  UTEST_ASSERT(ip_trace_length(trace) == 3,
	       "1st trace should contain 3 elements");
  UTEST_ASSERT(ip_trace_item_at(trace, 1)->elt.type == TRACE,
	       "1st trace's middle item should be a trace");
  UTEST_ASSERT(ip_trace_length(ip_trace_item_at(trace, 1)->elt.trace) == 3,
	       "1st trace's middle item should contain 3 elements");

  _array_get_at(traces, 1, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"2nd trace's status should be success");
  // Trace should contain 3 elements: 2 nodes, 1 trace
  // (0.0.0.1 [0.0.0.1 0.0.0.3 0.0.0.4] 0.0.0.4)
  UTEST_ASSERT(ip_trace_length(trace) == 3,
	       "2nd trace should contain 3 elements");
  UTEST_ASSERT(ip_trace_item_at(trace, 1)->elt.type == TRACE,
	       "2nd trace's middle item should be a trace");
  UTEST_ASSERT(ip_trace_length(ip_trace_item_at(trace, 1)->elt.trace) == 3,
	       "2nd trace's middle item should contain 3 elements");

  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_tunnel_unreach ]---------------
/**
 *    n0 ------------ n1
 *  0.0.0.1         0.0.0.2
 * 255.0.0.1       255.0.0.2
 *                255.0.0.255
 */
static int test_net_traces_recordroute_tunnel_unreach()
{
   ez_topo_t * eztopo= _ez_topo_line_ptp();
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  array_t * traces;

  node_add_tunnel(ez_topo_get_node(eztopo, 0), IPV4(255,0,0,2),
		  IPV4(255,0,0,1), NULL, NET_ADDR_ANY);
  node_add_tunnel(ez_topo_get_node(eztopo, 1), IPV4(255,0,0,1),
		  IPV4(255,0,0,2), NULL, NET_ADDR_ANY);
  ez_topo_igp_compute(eztopo, 1);
  node_add_iface(ez_topo_get_node(eztopo, 1), IPV4PFX(255,0,0,255,32),
		 NET_IFACE_LOOPBACK);
  node_rt_add_route(ez_topo_get_node(eztopo, 0), IPV4PFX(255,0,0,255,32),
		    IPV4PFX(255,0,0,1,32), NET_ADDR_ANY, 0, NET_ROUTE_STATIC);
  net_iface_set_enabled(ez_topo_get_link(eztopo, 0), 0);
  opts= ip_options_create();
  ip_options_set(opts, IP_OPT_ECMP);
  ip_options_set(opts, IP_OPT_TUNNEL);
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			   IPV4(255,0,0,255), 255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "traces should contain a single trace");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status != ESUCCESS,
	       "trace's status should not be success");
  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_net_traces_recordroute_tunnel_load ]-------------------
static int test_net_traces_recordroute_tunnel_load()
{
  return UTEST_SKIPPED;
}

// -----[ test_net_traces_recordroute_tunnel_qos ]-------------------
static int test_net_traces_recordroute_tunnel_qos()
{
  return UTEST_SKIPPED;
}

// -----[ test_net_traces_recordroute_tunnel_prefix ]----------------
static int test_net_traces_recordroute_tunnel_prefix()
{
  ez_topo_t * eztopo= _ez_topo_line_ptp();
  ip_opt_t * opts;
  ip_trace_t * trace= NULL;
  array_t * traces;
  net_subnet_t * subnet= subnet_create(IPV4(255,0,0,255), 30,
				       NET_SUBNET_TYPE_STUB);
  node_add_tunnel(ez_topo_get_node(eztopo, 0), IPV4(255,0,0,2),
		  IPV4(255,0,0,1), NULL, NET_ADDR_ANY);
  node_add_tunnel(ez_topo_get_node(eztopo, 1), IPV4(255,0,0,1),
		  IPV4(255,0,0,2), NULL, NET_ADDR_ANY);
  ez_topo_igp_compute(eztopo, 1);
  network_add_subnet(eztopo->network, subnet);
  net_link_create_ptmp(ez_topo_get_node(eztopo, 1), subnet,
		       IPV4(255,0,0,255), NULL);
  node_rt_add_route(ez_topo_get_node(eztopo, 0), IPV4PFX(255,0,0,255,30),
		    IPV4PFX(255,0,0,1,32), NET_ADDR_ANY, 0, NET_ROUTE_STATIC);
  node_rt_add_route(ez_topo_get_node(eztopo, 1), IPV4PFX(255,0,0,255,30),
		    IPV4PFX(255,0,0,255,32), NET_ADDR_ANY, 0, NET_ROUTE_STATIC);
  opts= ip_options_create();
  ip_options_set(opts, IP_OPT_TUNNEL);
  ip_options_alt_dest(opts, IPV4PFX(255,0,0,255,30));
  traces= icmp_trace_send(ez_topo_get_node(eztopo, 0),
			  IPV4(255,0,0,255), 255, opts);
  UTEST_ASSERT(traces != NULL, "traces should not be NULL");
  UTEST_ASSERT(_array_length(traces) == 1,
	       "traces should contain a single trace");
  _array_get_at(traces, 0, &trace);
  UTEST_ASSERT(trace->status == ESUCCESS,
		"trace's status should be success (%s)",
		network_strerror(trace->status));
  UTEST_ASSERT(ip_trace_length(trace) == 4,
		"trace's length should be 4");
  UTEST_ASSERT(ip_trace_item_at(trace, 0)->elt.node ==
		ez_topo_get_node(eztopo, 0),
		"first hop should be 0.0.0.1");
  UTEST_ASSERT(ip_trace_item_at(trace, 1)->elt.type == TRACE,
		"mid hop should be a tunnel trace");
  UTEST_ASSERT(ip_trace_item_at(trace, 2)->elt.node ==
		ez_topo_get_node(eztopo, 1),
		"last hop should be 0.0.0.2");
  ip_options_destroy(&opts);
  _array_destroy(&traces);
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP ATTRIBUTES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_attr_aspath ]-------------------------------------
static int test_bgp_attr_aspath()
{
  bgp_path_t * path= path_create();
  UTEST_ASSERT(path_length(path) == 0,
		"new path must have 0 length");
  path_destroy(&path);
  UTEST_ASSERT(path == NULL,
		"destroyed path must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_prepend ]-----------------------------
static int test_bgp_attr_aspath_prepend()
{
  bgp_path_t * path= path_create();
  UTEST_ASSERT(path_append(&path, 1) >= 0,
		"path_prepend() should return >= 0 on success");
  UTEST_ASSERT(path_length(path) == 1,
		"prepended path must have length of 1");
  path_destroy(&path);
  UTEST_ASSERT(path == NULL,
		"destroyed path must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_prepend_too_much ]--------------------
static int test_bgp_attr_aspath_prepend_too_much()
{
  unsigned int index;
  bgp_path_t * path= path_create();
  for (index= 0; index < MAX_PATH_SEQUENCE_LENGTH; index++) {
    UTEST_ASSERT(path_append(&path, index) >= 0,
		  "path_prepend() should return >= 0 on success");
  }
  UTEST_ASSERT(path_append(&path, 256) < 0,
		  "path_prepend() should return < 0 on failure");
  path_destroy(&path);
  UTEST_ASSERT(path == NULL,
		"destroyed path must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_2str ]--------------------------------
static int test_bgp_attr_aspath_2str()
{
  int result;
  bgp_path_t * path= NULL;
  char acBuffer[5]= "PLOP";
  // Empty path
  UTEST_ASSERT(path_to_string(path, 1, acBuffer, sizeof(acBuffer)) == 0,
		"path_to_string() should return 0");
  UTEST_ASSERT(strcmp(acBuffer, "") == 0,
		"string for null path must be \"\"");
  // Small path (fits in buffer)
  path= path_create();
  path_append(&path, 2611);
  result= path_to_string(path, 1, acBuffer, sizeof(acBuffer));
  UTEST_ASSERT(result == 4,
		"path_to_string() should return 4 (%d)", result);
  UTEST_ASSERT(strcmp(acBuffer, "2611") == 0,
		"string for path 2611 must be \"2611\"");
  // Larger path (does not fit in buffer)
  path_append(&path, 20965);
  result= path_to_string(path, 1, acBuffer, sizeof(acBuffer));
  UTEST_ASSERT(result < 0,
		"path_to_string() should return < 0 (%d)", result);
  path_destroy(&path);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_str2 ]--------------------------------
static int test_bgp_attr_aspath_str2()
{
  bgp_path_t * path;
  path= path_from_string("");
  UTEST_ASSERT(path != NULL, "\"\" is a valid AS-Path");
  path_destroy(&path);
  path= path_from_string("12 34 56");
  UTEST_ASSERT(path != NULL, "\"12 34 56\" is a valid AS-Path");
  path_destroy(&path);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_set_str2 ]----------------------------
static int test_bgp_attr_aspath_set_str2()
{
  bgp_path_t * path;
  bgp_path_seg_t * seg;
  path= path_from_string("{1 2 3}");
  UTEST_ASSERT(path != NULL, "\"{1 2 3}\" is a valid AS-Path");
  UTEST_ASSERT(path_num_segments(path) == 1,
		"Path should have a single segment");
  seg= (bgp_path_seg_t *) path->data[0];
  UTEST_ASSERT(seg->type == AS_PATH_SEGMENT_SET,
		"Segment type should be AS_SET");
  UTEST_ASSERT(seg->length == 3,
		"Segment size should be 3");
  UTEST_ASSERT((seg->asns[0] == 3) && (seg->asns[1] == 2) &&
		(seg->asns[2] == 1), "incorrect segment content");
  path_destroy(&path);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_cmp ]---------------------------------
static int test_bgp_attr_aspath_cmp()
{
  bgp_path_t * path1= NULL, * path2= NULL;
  UTEST_ASSERT(path_cmp(path1, path2) == 0,
		"null paths should be considered equal");
  path1= path_create();
  UTEST_ASSERT(path_cmp(path1, path2) == 0,
		"null and empty paths should be considered equal");
  path2= path_create();
  UTEST_ASSERT(path_cmp(path1, path2) == 0,
		"empty paths should be considered equal");
  path_append(&path1, 1);
  UTEST_ASSERT(path_cmp(path1, path2) == 1,
		"longest path should be considered better");
  path_append(&path2, 2);
  UTEST_ASSERT(path_cmp(path1, path2) == -1,
		"for equal-size paths, should be considered better");
  path_destroy(&path1);
  path_destroy(&path2);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_contains ]----------------------------
static int test_bgp_attr_aspath_contains()
{
  bgp_path_t * path= NULL;
  UTEST_ASSERT(path_contains(path, 1) == 0,
		"null path should not contain 1");
  path= path_create();
  UTEST_ASSERT(path_contains(path, 1) == 0,
		"empty path should not contain 1");
  path_append(&path, 2);
  UTEST_ASSERT(path_contains(path, 1) == 0,
		"path 2 should not contain 1");
  path_append(&path, 1);
  UTEST_ASSERT(path_contains(path, 1) != 0,
		"path 2 1 should contain 1");
  path_destroy(&path);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_rem_private ]-------------------------
static int test_bgp_attr_aspath_rem_private()
{
  bgp_path_t * path= NULL;
  path= path_from_string("64511 64512 12 34 65535");
  UTEST_ASSERT(path != NULL, "64511 64512 12 34 65535 is a valid AS-Path");
  UTEST_ASSERT(path_length(path) == 5, "path length should be 5");
  UTEST_ASSERT(path_contains(path, 64511), "path should contain 64511");
  UTEST_ASSERT(path_contains(path, 64512), "path should contain 64512");
  UTEST_ASSERT(path_contains(path, 12), "path should contain 12");
  UTEST_ASSERT(path_contains(path, 34), "path should contain 34");
  UTEST_ASSERT(path_contains(path, 65535), "path should contain 65535");
  path_remove_private(path);
  UTEST_ASSERT(path_length(path) == 3, "path length should be 3");
  UTEST_ASSERT(path_contains(path, 64511), "path should contain 64511");
  UTEST_ASSERT(path_contains(path, 64512) == 0,
		"path should not contain 64512");
  UTEST_ASSERT(path_contains(path, 12), "path should contain 12");
  UTEST_ASSERT(path_contains(path, 34), "path should contain 34");
  UTEST_ASSERT(path_contains(path, 65535) == 0,
		"path should not contain 65535");
  path_destroy(&path);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_aspath_match ]-------------------------------
static int test_bgp_attr_aspath_match()
{
  SRegEx * pRegEx= regex_init("^2611$", 0);
  bgp_path_t * path;
  UTEST_ASSERT(pRegEx != NULL, "reg-ex should not be NULL");
  path= path_from_string("2611");
  UTEST_ASSERT(path_match(path, pRegEx) != 0, "path should match");
  path_destroy(&path);
  regex_reinit(pRegEx);
  path= path_from_string("5511 2611");
  UTEST_ASSERT(path_match(path, pRegEx) == 0, "path should not match");
  path_destroy(&path);
  regex_finalize(&pRegEx);
  /* test for J. Schoenwalder (Bremen)
  pRegEx= regex_init("2|4", 0);
  path= path_create("1 2 3");
  UTEST_ASSERT(path_match(path, pRegEx) != 0, "path should match");
  path_destroy(&path);
  regex_reinit(pRegEx);
  path= path_from_string("1 2 3 4");
  UTEST_ASSERT(path_match(path, pRegEx) == 0, "path should not match");
  path_destroy(&path);
  regex_finalize(&pRegEx);*/
  UTEST_ASSERT(pRegEx == NULL, "destroyed reg-ex should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities ]--------------------------------
static int test_bgp_attr_communities()
{
  bgp_comms_t * comms= comms_create();
  comms_destroy(&comms);
  UTEST_ASSERT(comms == NULL, "destroyed Communities must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_append ]-------------------------
static int test_bgp_attr_communities_append()
{
  bgp_comms_t * comms= comms_create();
  UTEST_ASSERT(comms_contains(comms, 1) == 0,
		"comms_contains() should return == 0");
  UTEST_ASSERT(comms_add(&comms, 1) == 0,
		"comms_add() should return 0 on success");
  UTEST_ASSERT(comms_length(comms) == 1,
		"Communities length should be 1");
  UTEST_ASSERT(comms_contains(comms, 1) != 0,
		"comms_contains() should return != 0");
  comms_destroy(&comms);
  UTEST_ASSERT(comms == NULL, "destroyed Communities must be null");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_remove ]-------------------------
static int test_bgp_attr_communities_remove()
{
  bgp_comms_t * comms= comms_create();
  UTEST_ASSERT(comms_add(&comms, 1) == 0,
		"comms_add() should return 0 on success");
  UTEST_ASSERT(comms_length(comms) == 1,
		"Communities length should be 1");
  UTEST_ASSERT(comms_add(&comms, 2) == 0,
		"comms_add() should return 0 on success");
  UTEST_ASSERT(comms_length(comms) == 2,
		"Communities length should be 2");
  comms_remove(&comms, 1);
  UTEST_ASSERT(comms_length(comms) == 1,
		"Communities length should be 1");
  comms_remove(&comms, 2);
  UTEST_ASSERT(comms == NULL,
		"Communities should be destroyed when empty");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_2str ]---------------------------
static int test_bgp_attr_communities_2str()
{
  bgp_comms_t * comms= NULL;
  char acBuffer[10]= "PLOP";
  // Empty Communities
  UTEST_ASSERT(comm_to_string(comms, acBuffer, sizeof(acBuffer)) == 0,
		"comm_to_string() should return 0");
  UTEST_ASSERT(strcmp(acBuffer, "") == 0,
		"string for null Communities should be \"\"");
  // Small Communities (fits in buffer)
  comms= comms_create();
  comms_add(&comms, 1234);
  UTEST_ASSERT(comm_to_string(comms, acBuffer, sizeof(acBuffer)) == 4,
		"comm_to_string() should return 4");
  UTEST_ASSERT(strcmp(acBuffer, "1234") == 0,
		"string for Communities 1234 should be \"1234\"");
  // Larger Communities (still fits in buffer)
  comms_add(&comms, 2345);
  UTEST_ASSERT(comm_to_string(comms, acBuffer, sizeof(acBuffer)) == 9,
		"comm_to_string() should return == 9");
  UTEST_ASSERT(strcmp(acBuffer, "1234 2345") == 0,
		"string for Communities 1234,2345 should be \"1234 2345\"");
  // Even larger Communities (does not fit in buffer)
  comms_add(&comms, 3456);
  UTEST_ASSERT(comm_to_string(comms, acBuffer, sizeof(acBuffer)) < 0,
		"comm_to_string() should return < 0");
  comms_destroy(&comms);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_str2 ]---------------------------
static int test_bgp_attr_communities_str2()
{
  bgp_comms_t * comms;
  comms= comm_from_string("");
  UTEST_ASSERT(comms != NULL,
		"\"\" is a valid Communities");
  comms_destroy(&comms);
  comms= comm_from_string("1234:5678");
  UTEST_ASSERT(comms != NULL,
		"\"1234:5678\" is a valid Communities");
  comms_destroy(&comms);
  comms= comm_from_string("65535:65535");
  UTEST_ASSERT(comms != NULL,
		"\"1234:5678\" is a valid Communities");
  comms_destroy(&comms);
  comms= comm_from_string("12345678");
  UTEST_ASSERT(comms != NULL,
		"\"12345678\" is a valid Communities");
  comms_destroy(&comms);
  comms= comm_from_string("4294967295");
  UTEST_ASSERT(comms != NULL,
		"\"4294967295\" is a valid Communities");
  comms_destroy(&comms);
  comms= comm_from_string("1234:5678abc");
  UTEST_ASSERT(comms == NULL,
		"\"1234:5678abc\" is not a valid Communities");
  comms= comm_from_string("4294967296");
  UTEST_ASSERT(comms == NULL,
		"\"4294967296\" is not a valid Communities");
  comms= comm_from_string("65536:0");
  UTEST_ASSERT(comms == NULL,
		"\"65536:0\" is not a valid Communities");
  comms= comm_from_string("0:65536");
  UTEST_ASSERT(comms == NULL,
		"\"0:65536\" is not a valid Communities");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_cmp ]----------------------------
static int test_bgp_attr_communities_cmp()
{
  bgp_comms_t * comms1= NULL, * comms2= NULL;
  UTEST_ASSERT(comms_cmp(comms1, comms2) == 0,
		"null Communities should be considered equal");
  comms1= comms_create();
  UTEST_ASSERT(comms_cmp(comms1, comms2) == 0,
		"null and empty Communities should be considered equal");
  comms2= comms_create();
  UTEST_ASSERT(comms_cmp(comms1, comms2) == 0,
		"empty Communities should be considered equal");
  comms_add(&comms1, 1);
  UTEST_ASSERT(comms_cmp(comms1, comms2) == 1,
		"longest Communities should be considered first");
  comms_add(&comms2, 2);
  UTEST_ASSERT(comms_cmp(comms1, comms2) == -1,
		"in equal-size Communities, first largest value should win");
  comms_destroy(&comms1);
  comms_destroy(&comms2);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_communities_contains ]-----------------------
static int test_bgp_attr_communities_contains()
{
  bgp_comms_t * comms= NULL;
  UTEST_ASSERT(comms_contains(comms, 1) == 0,
		"null Communities should not contain 1");
  comms= comms_create();
  UTEST_ASSERT(comms_contains(comms, 1) == 0,
		"empty Communities should not contain 1");
  comms_add(&comms, 2);
  UTEST_ASSERT(comms_contains(comms, 1) == 0,
		"Communities 2 should not contain 1");
  comms_add(&comms, 1);
  UTEST_ASSERT(comms_contains(comms, 1) != 0,
		"Communities 2 1 should contain 1");
  comms_destroy(&comms);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_cluster_list ]-------------------------------
static int test_bgp_attr_cluster_list()
{
  bgp_cluster_list_t * cl= cluster_list_create();
  UTEST_ASSERT(cl != NULL,
		"new Cluster-Id-List should not be NULL");
  cluster_list_destroy(&cl);
  UTEST_ASSERT(cl == NULL,
		"Cluster-Id-List should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_cluster_list_append ]------------------------
static int test_bgp_attr_cluster_list_append()
{
  bgp_cluster_list_t * cl= cluster_list_create();
  UTEST_ASSERT(cl != NULL,
		"new Cluster-Id-List should not be NULL");
  UTEST_ASSERT(cluster_list_append(cl, IPV4(1,2,3,4)) >= 0,
		"cluster_list_append() should succeed");
  UTEST_ASSERT(cluster_list_length(cl) == 1,
		"Cluster-Id-List length should be 1");
  UTEST_ASSERT(cluster_list_append(cl, IPV4(5,6,7,8)) >= 0,
		"cluster_list_append() should succeed");
  UTEST_ASSERT(cluster_list_length(cl) == 2,
		"Cluster-Id-List length should be 2");
  UTEST_ASSERT(cluster_list_append(cl, IPV4(9,0,1,2)) >= 0,
		"cluster_list_append() should succeed");
  UTEST_ASSERT(cluster_list_length(cl) == 3,
		"Cluster-Id-List length should be 3");
  UTEST_ASSERT(cluster_list_append(cl, IPV4(3,4,5,6)) >= 0,
		"cluster_list_append() should succeed");
  UTEST_ASSERT(cluster_list_length(cl) == 4,
		"Cluster-Id-List length should be 4");
  cluster_list_destroy(&cl);
  UTEST_ASSERT(cl == NULL,
		"Cluster-Id-List should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_attr_cluster_list_contains ]----------------------
static int test_bgp_attr_cluster_list_contains()
{
  bgp_cluster_list_t * cl= cluster_list_create();
  cluster_list_append(cl, IPV4(1,2,3,4));
  cluster_list_append(cl, IPV4(5,6,7,8));
  cluster_list_append(cl, IPV4(9,0,1,2));
  UTEST_ASSERT(cluster_list_length(cl) == 3,
		"Cluster-Id-List length should be 3");
  UTEST_ASSERT(cluster_list_contains(cl, IPV4(1,2,3,4)) != 0,
		"Cluster-Id-List should contains 1.2.3.4");
  UTEST_ASSERT(cluster_list_contains(cl, IPV4(5,6,7,8))	!= 0,
		"Cluster-Id-List should contains 5.6.7.8");
  UTEST_ASSERT(cluster_list_contains(cl, IPV4(9,0,1,2)) != 0,
		"Cluster-Id-List should contains 9.0.1.2");
  UTEST_ASSERT(cluster_list_contains(cl, IPV4(2,1,0,9)) == 0,
		"Cluster-Id-List should not contain 2.1.0.9");
  UTEST_ASSERT(cluster_list_contains(cl, IPV4(0,0,0,0)) == 0,
		"Cluster-Id-List should not contain 0.0.0.0");
  UTEST_ASSERT(cluster_list_contains(cl, IPV4(255,255,255,255))
		== 0,
		"Cluster-Id-List should not contain 255.255.255.255");
  cluster_list_destroy(&cl);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP ROUTES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_route_basic ]-------------------------------------
static int test_bgp_route_basic()
{
  ip_pfx_t pfx= IPV4PFX(130,104,0,0,16);
  bgp_route_t * route= route_create(pfx, NULL, IPV4(1,0,0,0),
				    BGP_ORIGIN_IGP);
  UTEST_ASSERT(route != NULL,
		"Route should not be NULL when created");
  route_destroy(&route);
  UTEST_ASSERT(route == NULL,
		"Route should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_route_communities ]-------------------------------
static int test_bgp_route_communities()
{
  bgp_route_t * route1, * route2;
  bgp_comms_t * comm1, * comm2;

  ip_pfx_t pfx= IPV4PFX(130,104,0,0,16);

  route1= route_create(pfx, NULL, IPV4(1,0,0,0), BGP_ORIGIN_IGP);
  route2= route_create(pfx, NULL, IPV4(2,0,0,0), BGP_ORIGIN_IGP);

  // Copy of Communities must be different
  comm1= comms_create();
  comm2= comms_dup(comm1);
  UTEST_ASSERT(comm1 != comm2,
		"Communities copy should not be equal to original");

  // Communities must not exist in global repository
  UTEST_ASSERT((comm_hash_get(comm1) == NULL) &&
		(comm_hash_get(comm2) == NULL),
		"Communities should not already be interned");

  // Communities must now be interned and equal
  route_set_comm(route1, comm1);
  route_set_comm(route2, comm2);
  UTEST_ASSERT(route1->attr->comms == route2->attr->comms,
		"interned Communities not equal");
  UTEST_ASSERT((comm_hash_get(route1->attr->comms) ==
		 route1->attr->comms) &&
		(comm_hash_get(route2->attr->comms) ==
		 route2->attr->comms),
		"Communities <-> hash mismatch");
  UTEST_ASSERT(comm_hash_refcnt(route1->attr->comms) == 2,
		"Incorrect reference count");

  // Append community value
  route_comm_append(route1, 12345);
  UTEST_ASSERT(route1->attr->comms != route2->attr->comms,
		"interned Communities are equal");
  UTEST_ASSERT((comm_hash_get(route1->attr->comms) ==
		 route1->attr->comms) &&
		(comm_hash_get(route2->attr->comms) ==
		 route2->attr->comms),
		"Communities <-> hash mismatch");

  // Append Community value
  route_comm_append(route2, 12345);
  UTEST_ASSERT(route1->attr->comms == route2->attr->comms,
		"interned Communities not equal");
  UTEST_ASSERT((comm_hash_get(route1->attr->comms) ==
		 route1->attr->comms) &&
		(comm_hash_get(route2->attr->comms) ==
		 route2->attr->comms),
		"Communities <-> hash mismatch");

  route_destroy(&route1);
  route_destroy(&route2);

  return EXIT_SUCCESS;
}

// -----[ test_bgp_route_aspath ]------------------------------------
static int test_bgp_route_aspath()
{
  bgp_route_t * route1, * route2;
  bgp_path_t * path1, * path2;

  ip_pfx_t pfx= IPV4PFX(130,104,0,0,16);

  route1= route_create(pfx, NULL, IPV4(1,0,0,0), BGP_ORIGIN_IGP);
  route2= route_create(pfx, NULL, IPV4(2,0,0,0), BGP_ORIGIN_IGP);

  path1= path_create();
  path2= path_copy(path1);
  UTEST_ASSERT(path1 != path2,
		"AS-Path copy should not be equal to original");

  route_set_path(route1, path1);
  route_set_path(route2, path2);
  UTEST_ASSERT(route1->attr->path_ref == route2->attr->path_ref,
		"AS-Paths should not already be internet");

  // Prepend first route's AS-Path
  route_path_prepend(route1, 666, 2);
  UTEST_ASSERT(route1->attr->path_ref != route2->attr->path_ref,
		"");

  // Prepend second route's AS-Path
  route_path_prepend(route2, 666, 2);
  UTEST_ASSERT(route1->attr->path_ref == route2->attr->path_ref,
		"");

  // Destroy route 1
  route_destroy(&route1);

  // Destroy route 2
  route_destroy(&route2);

  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP FILTER ACTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_filter_action_comm_add ]--------------------------
int test_bgp_filter_action_comm_add()
{
  bgp_ft_action_t * action= filter_action_comm_append(1234);
  UTEST_ASSERT(action != NULL, "New action should not be NULL");
  UTEST_ASSERT(action->code == FT_ACTION_COMM_APPEND,
		"Incorrect action code");
  filter_action_destroy(&action);
  UTEST_ASSERT(action == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_remove ]-----------------------
int test_bgp_filter_action_comm_remove()
{
  bgp_ft_action_t * action= filter_action_comm_remove(1234);
  UTEST_ASSERT(action != NULL, "New action should not be NULL");
  UTEST_ASSERT(action->code == FT_ACTION_COMM_REMOVE,
		"Incorrect action code");
  filter_action_destroy(&action);
  UTEST_ASSERT(action == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_strip ]------------------------
int test_bgp_filter_action_comm_strip()
{
  bgp_ft_action_t * action= filter_action_comm_strip();
  UTEST_ASSERT(action != NULL, "New action should not be NULL");
  UTEST_ASSERT(action->code == FT_ACTION_COMM_STRIP,
		"Incorrect action code");
  filter_action_destroy(&action);
  UTEST_ASSERT(action == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_local_pref ]------------------------
int test_bgp_filter_action_local_pref()
{
  bgp_ft_action_t * action= filter_action_pref_set(1234);
  UTEST_ASSERT(action != NULL, "New action should not be NULL");
  UTEST_ASSERT(action->code == FT_ACTION_PREF_SET,
		"Incorrect action code");
  filter_action_destroy(&action);
  UTEST_ASSERT(action == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_metric ]----------------------------
int test_bgp_filter_action_metric()
{
  bgp_ft_action_t * action= filter_action_metric_set(1234);
  UTEST_ASSERT(action != NULL, "New action should not be NULL");
  UTEST_ASSERT(action->code == FT_ACTION_METRIC_SET,
		"Incorrect action code");
  filter_action_destroy(&action);
  UTEST_ASSERT(action == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_metric_internal ]-------------------
int test_bgp_filter_action_metric_internal()
{
  bgp_ft_action_t * action= filter_action_metric_internal();
  UTEST_ASSERT(action != NULL, "New action should not be NULL");
  UTEST_ASSERT(action->code == FT_ACTION_METRIC_INTERNAL,
		"Incorrect action code");
  filter_action_destroy(&action);
  UTEST_ASSERT(action == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_path_prepend ]----------------------
int test_bgp_filter_action_path_prepend()
{
  bgp_ft_action_t * action= filter_action_path_prepend(5);
  UTEST_ASSERT(action != NULL, "New action should not be NULL");
  UTEST_ASSERT(action->code == FT_ACTION_PATH_PREPEND,
		"Incorrect action code");
  filter_action_destroy(&action);
  UTEST_ASSERT(action == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_path_insert ]-----------------------
int test_bgp_filter_action_path_insert()
{
  bgp_ft_action_t * action= filter_action_path_insert(1234, 3);
  UTEST_ASSERT(action != NULL, "New action should not be NULL");
  UTEST_ASSERT(action->code == FT_ACTION_PATH_INSERT,
	       "Incorrect action code");
  filter_action_destroy(&action);
  UTEST_ASSERT(action == NULL, "Action should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_add_str2 ]---------------------
int test_bgp_filter_action_comm_add_str2()
{
  bgp_ft_action_t * action;
  UTEST_ASSERT(filter_parser_action("community add 1234", &action) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&action);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_remove_str2 ]------------------
int test_bgp_filter_action_comm_remove_str2()
{
  bgp_ft_action_t * action;
  UTEST_ASSERT(filter_parser_action("community remove 1234", &action) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&action);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_comm_strip_str2 ]-------------------
int test_bgp_filter_action_comm_strip_str2()
{
  bgp_ft_action_t * action;
  UTEST_ASSERT(filter_parser_action("community strip", &action) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&action);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_local_pref_str2 ]-------------------
int test_bgp_filter_action_local_pref_str2()
{
  bgp_ft_action_t * action;
  UTEST_ASSERT(filter_parser_action("local-pref 1234", &action) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&action);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_metric_str2 ]-----------------------
int test_bgp_filter_action_metric_str2()
{
  bgp_ft_action_t * action;
  UTEST_ASSERT(filter_parser_action("metric 1234", &action) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&action);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_metric_internal_str2 ]--------------
int test_bgp_filter_action_metric_internal_str2()
{
  bgp_ft_action_t * action;
  UTEST_ASSERT(filter_parser_action("metric internal", &action) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&action);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_path_prepend_str2 ]-----------------
int test_bgp_filter_action_path_prepend_str2()
{
  bgp_ft_action_t * action;
  UTEST_ASSERT(filter_parser_action("as-path prepend 5", &action) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&action);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_action_expression_str2 ]-------------------
int test_bgp_filter_action_expression_str2()
{
  bgp_ft_action_t * action;
  UTEST_ASSERT(filter_parser_action("as-path prepend 2, local-pref 253",
				     &action) == 0,
		"filter_parser_action() should return 0");
  filter_action_destroy(&action);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP FILTER PREDICATES
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_filter_predicate_comm_contains ]------------------
int test_bgp_filter_predicate_comm_contains()
{
  bgp_ft_matcher_t * predicate= filter_match_comm_contains(1234);
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_COMM_CONTAINS,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  UTEST_ASSERT(predicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_nexthop_is ]---------------------
int test_bgp_filter_predicate_nexthop_is()
{
  bgp_ft_matcher_t * predicate=
    filter_match_nexthop_equals(IPV4(10,0,0,0));
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_NEXTHOP_IS,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  UTEST_ASSERT(predicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_nexthop_in ]---------------------
int test_bgp_filter_predicate_nexthop_in()
{
  ip_pfx_t pfx= IPV4PFX(10,0,0,0,16);
  bgp_ft_matcher_t * predicate=
    filter_match_nexthop_in(pfx);
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_NEXTHOP_IN,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  UTEST_ASSERT(predicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_is ]----------------------
int test_bgp_filter_predicate_prefix_is()
{
  ip_pfx_t pfx= IPV4PFX(10,0,0,0,16);
  bgp_ft_matcher_t * predicate=
    filter_match_prefix_equals(pfx);
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_PREFIX_IS,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  UTEST_ASSERT(predicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_in ]----------------------
int test_bgp_filter_predicate_prefix_in()
{
  ip_pfx_t pfx= IPV4PFX(10,0,0,0,16);
  bgp_ft_matcher_t * predicate=
    filter_match_prefix_in(pfx);
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_PREFIX_IN,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  UTEST_ASSERT(predicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_ge ]----------------------
static int test_bgp_filter_predicate_prefix_ge()
{
  ip_pfx_t pfx= IPV4PFX(10,0,0,0,16);
  bgp_ft_matcher_t * predicate=
    filter_match_prefix_ge(pfx, 24);
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_PREFIX_GE,
		"Incorrect predicate code");
  UTEST_ASSERT(*((uint8_t *) (predicate->params+sizeof(ip_pfx_t))) == 24,
		"Incorrect prefix length");
  filter_matcher_destroy(&predicate);
  UTEST_ASSERT(predicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_le ]----------------------
static int test_bgp_filter_predicate_prefix_le()
{
  ip_pfx_t pfx= IPV4PFX(10,0,0,0,16);
  bgp_ft_matcher_t * predicate=
    filter_match_prefix_le(pfx, 24);
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_PREFIX_LE,
		"Incorrect predicate code");
  UTEST_ASSERT(*((uint8_t *) (predicate->params+sizeof(ip_pfx_t))) == 24,
		"Incorrect prefix length");
  filter_matcher_destroy(&predicate);
  UTEST_ASSERT(predicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_path_regexp ]--------------------
int test_bgp_filter_predicate_path_regexp()
{
  int regex_index= 12345;
  bgp_ft_matcher_t * predicate=
    filter_match_path(regex_index);
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_PATH_MATCHES,
		"Incorrect predicate code");
  UTEST_ASSERT(memcmp(predicate->params, &regex_index,
		       sizeof(regex_index)) == 0,
		"Incorrect regexp index");
  filter_matcher_destroy(&predicate);
  UTEST_ASSERT(predicate == NULL,
		"Predicate should be NULL when destroyed");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_and ]----------------------------
int test_bgp_filter_predicate_and()
{
  bgp_ft_matcher_t * predicate=
    filter_match_and(filter_match_comm_contains(1),
		     filter_match_comm_contains(2));
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_OP_AND,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_and_any ]------------------------
int test_bgp_filter_predicate_and_any()
{
  bgp_ft_matcher_t * predicate=
    filter_match_and(NULL,
		     filter_match_comm_contains(2));
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_COMM_CONTAINS,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_not ]----------------------------
int test_bgp_filter_predicate_not()
{
  bgp_ft_matcher_t * predicate=
    filter_match_not(filter_match_comm_contains(1));
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_OP_NOT,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_not_not ]------------------------
int test_bgp_filter_predicate_not_not()
{
  bgp_ft_matcher_t * predicate=
    filter_match_not(filter_match_not(filter_match_comm_contains(1)));
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_COMM_CONTAINS,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_or ]----------------------------
int test_bgp_filter_predicate_or()
{
  bgp_ft_matcher_t * predicate=
    filter_match_or(filter_match_comm_contains(1),
		    filter_match_comm_contains(2));
  UTEST_ASSERT(predicate != NULL,
		"New predicate should not be NULL");
  UTEST_ASSERT(predicate->code == FT_MATCH_OP_OR,
		"Incorrect predicate code");
  filter_matcher_destroy(&predicate);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_or_any ]------------------------
int test_bgp_filter_predicate_or_any()
{
  bgp_ft_matcher_t * predicate=
    filter_match_or(NULL,
		    filter_match_comm_contains(2));
  UTEST_ASSERT(predicate == NULL,
		"New predicate should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_comm_contains_str2 ]-------------
int test_bgp_filter_predicate_comm_contains_str2()
{
  bgp_ft_matcher_t * matcher;
  UTEST_ASSERT(predicate_parser("community is 1", &matcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&matcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_nexthop_is_str2 ]----------------
int test_bgp_filter_predicate_nexthop_is_str2()
{
  bgp_ft_matcher_t * matcher;
  UTEST_ASSERT(predicate_parser("next-hop is 1.0.0.0", &matcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&matcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_nexthop_in_str2 ]----------------
int test_bgp_filter_predicate_nexthop_in_str2()
{
  bgp_ft_matcher_t * matcher;
  UTEST_ASSERT(predicate_parser("next-hop in 10/8", &matcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&matcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_is_str2 ]-----------------
int test_bgp_filter_predicate_prefix_is_str2()
{
  bgp_ft_matcher_t * matcher;
  UTEST_ASSERT(predicate_parser("prefix is 10/8", &matcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&matcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_in_str2 ]-----------------
int test_bgp_filter_predicate_prefix_in_str2()
{
  bgp_ft_matcher_t * matcher;
  UTEST_ASSERT(predicate_parser("prefix in 10/8", &matcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&matcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_ge_str2 ]-----------------
static int test_bgp_filter_predicate_prefix_ge_str2()
{
  bgp_ft_matcher_t * matcher;
  int result;
  UTEST_ASSERT((result= predicate_parser("prefix ge 10/8 16", &matcher)) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0 (%d) => \"%s\"", result,
		  predicate_parser_strerror(result));
  filter_matcher_destroy(&matcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_prefix_le_str2 ]-----------------
static int test_bgp_filter_predicate_prefix_le_str2()
{
  bgp_ft_matcher_t * matcher;
  UTEST_ASSERT(predicate_parser("prefix le 10/8 16", &matcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&matcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_path_regexp_str2 ]---------------
int test_bgp_filter_predicate_path_regexp_str2()
{
  bgp_ft_matcher_t * matcher;
  UTEST_ASSERT(predicate_parser("path \"^(1_)\"", &matcher) ==
		PREDICATE_PARSER_SUCCESS,
		"predicate_parser() should return 0");
  filter_matcher_destroy(&matcher);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_filter_predicate_expr_str2 ]----------------------
int test_bgp_filter_predicate_expr_str2()
{
  char * predicates[]= {
    "(community is 1)",
    "(community is 1) | (community is 2)",
    "(path \"^(1_)\") & (community is 1) | (community is 2)",
    "(path \"^(1_)\") & ((community is 1) | (community is 2))",
    "!community is 1",
    "!(community is 1)",
    "(community is 1 & community is 2)",
    "!(!community is 1)",
    "((community is 1))",
  };
  bgp_ft_matcher_t * matcher;
  unsigned int index;

  for (index= 0; index < sizeof(predicates)/sizeof(predicates[0]);
       index++) {
    UTEST_ASSERT(predicate_parser(predicates[index], &matcher) >= 0,
		  "[%u] predicate_parser() should return >= 0", index);
    UTEST_ASSERT(matcher != NULL,
		  "New predicate should not be NULL");
    filter_matcher_destroy(&matcher);
    UTEST_ASSERT(matcher == NULL, "Predicate should be NULL when destroyed");
  }

  UTEST_ASSERT(predicate_parser("path \"^(1_)", &matcher) ==
		PREDICATE_PARSER_ERROR_UNFINISHED_STRING,
		"Should return \"unfinished-string\" error");
  UTEST_ASSERT(predicate_parser("plop", &matcher) ==
		PREDICATE_PARSER_ERROR_ATOM,
		"Should return \"atom\" error");
  UTEST_ASSERT(predicate_parser("(community is 1", &matcher) ==
		PREDICATE_PARSER_ERROR_PAR_MISMATCH,
		"Should return \"parenthesis-mismatch\" error");
  UTEST_ASSERT(predicate_parser("community is 1)", &matcher) ==
		PREDICATE_PARSER_ERROR_PAR_MISMATCH,
		"Should return \"parenthesis-mismatch\" error");
  UTEST_ASSERT(predicate_parser("& community is 1", &matcher) ==
		PREDICATE_PARSER_ERROR_LEFT_EXPR,
		"Should return \"left-expression\" error");
  UTEST_ASSERT(predicate_parser("community is 1 &", &matcher) ==
		PREDICATE_PARSER_ERROR_UNEXPECTED_EOF,
		"Should return \"unexpected-eof\" error");
  UTEST_ASSERT(predicate_parser("!!community is 1", &matcher) ==
		PREDICATE_PARSER_ERROR_UNARY_OP,
		"Should return \"unary-op-not-allowed\" error");
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP ROUTER
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_router ]------------------------------------------
static int test_bgp_router()
{
  net_node_t * node= __node_create(IPV4(1,0,0,0));
  bgp_router_t * router;
  UTEST_ASSERT(bgp_router_create(2611, node, &router) == ESUCCESS,
	       "router creation should succeed");
  UTEST_ASSERT(router != NULL, "router creation should succeed");
  UTEST_ASSERT(router->node == node,
		"incorrect underlying node reference");
  UTEST_ASSERT(router->asn == 2611, "incorrect ASN");
  UTEST_ASSERT(router->loc_rib != NULL,
		"LocRIB not properly initialized");
  UTEST_ASSERT(router->local_nets != NULL,
		"local-networks not properly initialized");
  bgp_router_destroy(&router);
  UTEST_ASSERT(router == NULL, "destroyed router should be NULL");
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_router_no_iface ]---------------------------------
static int test_bgp_router_no_iface()
{
  net_node_t * node;
  bgp_router_t * router;
  assert(node_create(IPV4(1,0,0,0), &node, 0) == ESUCCESS);
  UTEST_ASSERT(bgp_router_create(2611, node, &router) == ENET_IFACE_UNKNOWN,
	       "should return \"unknown interface\" error");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_router_add_network ]------------------------------
static int test_bgp_router_add_network()
{
  net_node_t * node= __node_create(IPV4(1,0,0,0));
  bgp_router_t * router;
  UTEST_ASSERT(bgp_router_create(2611, node, &router) == ESUCCESS,
	       "router creation should succeed");
  ip_pfx_t pfx= IPV4PFX(192,168,0,0,24);
  UTEST_ASSERT(bgp_router_add_network(router, pfx) == ESUCCESS,
		"addition of network should succeed");
  bgp_router_destroy(&router);
  node_destroy(&node);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_router_add_network_dup ]--------------------------
static int test_bgp_router_add_network_dup()
{
  net_node_t * node= __node_create(IPV4(1,0,0,0));
  bgp_router_t * router;
  UTEST_ASSERT(bgp_router_create(2611, node, &router) == ESUCCESS,
	       "router creation should succeed");
  ip_pfx_t pfx= IPV4PFX(192,168,0,0,24);
  UTEST_ASSERT(bgp_router_add_network(router, pfx) == ESUCCESS,
		"addition of network should succeed");
  UTEST_ASSERT(bgp_router_add_network(router, pfx)
		== EBGP_NETWORK_DUPLICATE,
		"addition of duplicate network should fail");
  bgp_router_destroy(&router);
  node_destroy(&node);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP PEER
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_peer ]--------------------------------------------
static int test_bgp_peer()
{
  net_addr_t peer_addr= IPV4(1,2,3,4);
  bgp_peer_t * peer= bgp_peer_create(1234, peer_addr, NULL);
  UTEST_ASSERT(peer != NULL,
		"bgp peer creation should succeed");
  UTEST_ASSERT(peer->addr == peer_addr,
		"peer address is incorrect");
  UTEST_ASSERT(peer->src_addr == NET_ADDR_ANY,
		"default source address should be 0.0.0.0"); 
  UTEST_ASSERT(peer->next_hop == NET_ADDR_ANY,
		"default next-hop address should be 0.0.0.0");
  UTEST_ASSERT(peer->session_state == SESSION_STATE_IDLE,
		"session state should be IDLE");
  UTEST_ASSERT((peer->filter[FILTER_IN] == NULL) &&
		(peer->filter[FILTER_OUT] == NULL),
		"default in/out-filters should be NULL (accept any)");
  UTEST_ASSERT((peer->adj_rib[RIB_IN] != NULL) &&
		(peer->adj_rib[RIB_OUT] != NULL),
		"Adj-RIB-in/out is not initialized");
  UTEST_ASSERT(peer->last_error == ESUCCESS,
		"last-error should be SUCCESS");
  bgp_peer_destroy(&peer);
  UTEST_ASSERT(peer == NULL,
		"destroyed peer should be NULL");
  return UTEST_SUCCESS;
}

// -----[ test_bgp_peer_open ]---------------------------------------
static int test_bgp_peer_open()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  bgp_router_t * router1, * router2;
  bgp_peer_t * peer1, * peer2;
  ez_topo_igp_compute(eztopo, 1);
  bgp_add_router(2611, ez_topo_get_node(eztopo, 0), &router1);
  bgp_add_router(2611, ez_topo_get_node(eztopo, 1), &router2);
  bgp_router_add_peer(router1, 2611, ez_topo_get_node(eztopo, 1)->rid, &peer1);
  bgp_router_add_peer(router2, 2611, ez_topo_get_node(eztopo, 0)->rid, &peer2);
  bgp_peer_open_session(peer1);
  bgp_peer_open_session(peer2);
  UTEST_ASSERT(peer1->session_state == SESSION_STATE_OPENWAIT,
		"session state should be OPENWAIT");
  UTEST_ASSERT(peer2->session_state == SESSION_STATE_OPENWAIT,
		"session state should be OPENWAIT");
  ez_topo_sim_run(eztopo);
  UTEST_ASSERT(peer1->session_state == SESSION_STATE_ESTABLISHED,
		"session state should be ESTABLISHED");
  UTEST_ASSERT(peer2->session_state == SESSION_STATE_ESTABLISHED,
		"session state should be ESTABLISHED");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_peer_open_error_unreach ]-------------------------
static int test_bgp_peer_open_error_unreach()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  bgp_router_t * router1, * router2;
  bgp_peer_t * peer1, * peer2;
  bgp_add_router(2611, ez_topo_get_node(eztopo, 0), &router1);
  bgp_add_router(2611, ez_topo_get_node(eztopo, 1), &router2);
  bgp_router_add_peer(router1, 2611, ez_topo_get_node(eztopo, 1)->rid, &peer1);
  bgp_router_add_peer(router2, 2611, ez_topo_get_node(eztopo, 0)->rid, &peer2);
  bgp_peer_open_session(peer1);
  bgp_peer_open_session(peer2);
  UTEST_ASSERT(peer1->session_state == SESSION_STATE_ACTIVE,
		"session state should be ACTIVE");
  UTEST_ASSERT(peer1->last_error == ENET_HOST_UNREACH,
		"last-error should be host-unreach");
  UTEST_ASSERT(peer2->session_state == SESSION_STATE_ACTIVE,
		"session state should be ACTIVE");
  UTEST_ASSERT(peer2->last_error == ENET_HOST_UNREACH,
		"last-error should be host-unreach");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_peer_open_error_proto ]---------------------------
static int test_bgp_peer_open_error_proto()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  bgp_router_t * router1;
  bgp_peer_t * peer1;
  ez_topo_igp_compute(eztopo, 1);
  bgp_add_router(2611, ez_topo_get_node(eztopo, 0), &router1);
  bgp_router_add_peer(router1, 2611, ez_topo_get_node(eztopo, 1)->rid, &peer1);
  bgp_peer_open_session(peer1);
  ez_topo_sim_run(eztopo);
  UTEST_ASSERT(peer1->session_state == SESSION_STATE_OPENWAIT,
		"session state should be OPENWAIT");
  ez_topo_sim_run(eztopo); // consumes ICMP error message (dst proto unreach)
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}

// -----[ test_bgp_peer_close ]--------------------------------------
static int test_bgp_peer_close()
{
  ez_topo_t * eztopo= _ez_topo_line_rtr();
  bgp_router_t * router1, * router2;
  bgp_peer_t * peer1, * peer2;
  ez_topo_igp_compute(eztopo, 1);
  bgp_add_router(2611, ez_topo_get_node(eztopo, 0), &router1);
  bgp_add_router(2611, ez_topo_get_node(eztopo, 1), &router2);
  bgp_router_add_peer(router1, 2611, ez_topo_get_node(eztopo, 1)->rid, &peer1);
  bgp_router_add_peer(router2, 2611, ez_topo_get_node(eztopo, 0)->rid, &peer2);
  bgp_peer_open_session(peer1);
  bgp_peer_open_session(peer2);
  ez_topo_sim_run(eztopo);
  UTEST_ASSERT(peer1->session_state == SESSION_STATE_ESTABLISHED,
		"session state should be ESTABLISHED");
  UTEST_ASSERT(peer2->session_state == SESSION_STATE_ESTABLISHED,
		"session state should be ESTABLISHED");
  bgp_peer_close_session(peer1);
  bgp_peer_close_session(peer2);
  ez_topo_sim_run(eztopo);
  UTEST_ASSERT(peer1->session_state == SESSION_STATE_IDLE,
		"session state should be IDLE");
  UTEST_ASSERT(peer2->session_state == SESSION_STATE_IDLE,
		"session state should be IDLE");
  ez_topo_destroy(&eztopo);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// BGP DOMAIN
//
/////////////////////////////////////////////////////////////////////

// -----[ test_bgp_domain_full_mesh ]--------------------------------
static int test_bgp_domain_full_mesh()
{
  return UTEST_SUCCESS;
}

// -----[ test_bgp_domain_full_mesh_ptp ]----------------------------
static int test_bgp_domain_full_mesh_ptp()
{
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// MRTD
//
/////////////////////////////////////////////////////////////////////

// -----[ test_mrtd_parse ]------------------------------------------
static int test_mrtd_parse()
{
  net_addr_t peer_addr;
  asn_t peer_asn;
  bgp_route_t * route;
  int result;
  bgp_comms_t * comms;
  bgp_path_t * path;

  result= mrtd_route_from_line("TABLE_DUMP|1122859488|B|198.32.12.9|11537|"
			       "128.61.32.0/19|10490 2637|IGP|199.77.193.9|"
			       "200|0|11537:950|NAG||",
			       &peer_addr, &peer_asn, &route);
  UTEST_ASSERT(result >= 0, "parse should succeed (%d)", result);
  UTEST_ASSERT(route != NULL, "returned route should be != NULL");
  UTEST_ASSERT(peer_addr == IPV4(198,32,12,9),
		"peer address incorrectly converted");
  UTEST_ASSERT(peer_asn == 11537,
		"peer ASN incorrectly converted");
  UTEST_ASSERT((route->prefix.network == IPV4(128,61,32,0)) &&
		(route->prefix.mask == 19),
		"prefix incorrectly converted");
  path= path_from_string("10490 2637");
  UTEST_ASSERT(path_equals(route->attr->path_ref, path),
	       "AS-path incorrecly converted");
  path_destroy(&path);
  UTEST_ASSERT(route->attr->origin == BGP_ORIGIN_IGP,
		"origin incorrectly converted");
  UTEST_ASSERT(route->attr->next_hop == IPV4(199,77,193,9),
		"nexthop incorrectly converted");
  UTEST_ASSERT(route->attr->local_pref == 200,
		"local-pref incorrectly converted");
  UTEST_ASSERT(route->attr->med == 0,
		"MED incorrectly converted");
  comms= comm_from_string("11537:950");
  UTEST_ASSERT(comms_equals(route->attr->comms, comms),
		"communities incorrectly converted");
  comms_destroy(&comms);
  route_destroy(&route);
  return  UTEST_SUCCESS;
}

// -----[ test_mrtd_parse_miss_fields ]------------------------------
static int test_mrtd_parse_miss_fields()
{
  int result= mrtd_route_from_line("TABLE_DUMP|1122859488|1|198.32.12.9|",
				   NULL, NULL, NULL);
  UTEST_ASSERT(result == MRTD_MISSING_FIELDS,
	       "should fail with not-enough-fields");
  return UTEST_SUCCESS;
}

// -----[ test_mrtd_parse_inv_header ]-------------------------------
static int test_mrtd_parse_inv_header()
{
  int result= mrtd_route_from_line("TABLE_DUMP|1122859488|1|198.32.12.9|11537|"
				   "128.61.32.0/19|10490 2637|IGP|"
				   "199.77.193.9|200|0|11537:950|NAG||",
				   NULL, NULL, NULL);
  UTEST_ASSERT(result == MRTD_INVALID_RECORD_TYPE,
	       "should fail with invalid-header");
  return UTEST_SUCCESS;
}

// -----[ test_mrtd_parse_inv_peerip ]-------------------------------
static int test_mrtd_parse_inv_peerip()
{
  int result= mrtd_route_from_line("TABLE_DUMP|1122859488|B|198-32.12.9|11537|"
				   "128.61.32.0/19|10490 2637|IGP|"
				   "199.77.193.9|200|0|11537:950|NAG||",
				   NULL, NULL, NULL);
  UTEST_ASSERT(result == MRTD_INVALID_PEER_ADDR,
	       "should fail with invalid-peer-IP");
  return UTEST_SUCCESS;
}

// -----[ test_mrtd_parse_inv_peerasn ]------------------------------
static int test_mrtd_parse_inv_peerasn()
{
  int result= mrtd_route_from_line("TABLE_DUMP|1122859488|B|198.32.12.9|"
				   "181537|128.61.32.0/19|10490 2637|IGP|"
				   "199.77.193.9|200|0|11537:950|NAG||",
				   NULL, NULL, NULL);
  UTEST_ASSERT(result == MRTD_INVALID_PEER_ASN,
	       "should fail with invalid-peer-ASN");
  return UTEST_SUCCESS;
}

// -----[ test_mrtd_parse_inv_prefix ]------------------------------
static int test_mrtd_parse_inv_prefix()
{
  int result= mrtd_route_from_line("TABLE_DUMP|1122859488|B|198.32.12.9|11537|"
				   "128.61.32.0,19|10490 2637|IGP|"
				   "199.77.193.9|200|0|11537:950|NAG||",
				   NULL, NULL, NULL);
  UTEST_ASSERT(result == MRTD_INVALID_PREFIX,
	       "should fail with invalid-prefix");
  return UTEST_SUCCESS;
}

// -----[ test_mrtd_parse_inv_origin ]------------------------------
static int test_mrtd_parse_inv_origin()
{
  int result= mrtd_route_from_line("TABLE_DUMP|1122859488|B|198.32.12.9|11537|"
				   "128.61.32.0/19|10490 2637|PLOP|"
				   "199.77.193.9|200|0|11537:950|NAG||",
				   NULL, NULL, NULL);
  UTEST_ASSERT(result == MRTD_INVALID_ORIGIN,
	       "should fail with invalid-origin");
  return UTEST_SUCCESS;
}

// -----[ test_mrtd_parse_inv_nexthop ]------------------------------
static int test_mrtd_parse_inv_nexthop()
{
  int result= mrtd_route_from_line("TABLE_DUMP|1122859488|B|198.32.12.9|11537|"
				   "128.61.32.0/19|10490 2637|IGP|"
				   "AAAAA|200|0|11537:950|NAG||",
				   NULL, NULL, NULL);
  UTEST_ASSERT(result == MRTD_INVALID_NEXTHOP,
	       "should fail with invalid-nexthop");
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// CLI
//
/////////////////////////////////////////////////////////////////////

// -----[ test_cli_empty ]-------------------------------------------
static int test_cli_empty()
{
  UTEST_ASSERT(libcbgp_exec_cmd("") == ESUCCESS,
		"libcbgp_exec_cmd() should succeed");
  return UTEST_SUCCESS;
}

// -----[ test_cli_comment ]-----------------------------------------
static int test_cli_comment()
{
  UTEST_ASSERT(libcbgp_exec_cmd("# This is a comment") == ESUCCESS,
		"libcbgp_exec_cmd() should succeed");
  return UTEST_SUCCESS;
}

// -----[ test_cli_error ]-------------------------------------------
static int test_cli_error()
{
  UTEST_ASSERT(libcbgp_exec_cmd("this is not a command") != ESUCCESS,
		"libcbgp_exec_cmd() should fail");
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// TRAFFIC PART
//
/////////////////////////////////////////////////////////////////////

// -----[ test_traffic_replay ]--------------------------------------
static int test_traffic_replay()
{
  ez_topo_t * topo= _ez_topo_triangle_rtr();
  ez_topo_igp_compute(topo, 1);

  UTEST_ASSERT(node_load_flow(ez_topo_get_node(topo, 0), IP_ADDR_ANY,
			      ez_topo_get_node(topo, 1)->rid,
			      1234, NULL, NULL, NULL) == ESUCCESS,
	       "node_load_flow() should succeed");

  UTEST_ASSERT(net_iface_get_load(ez_topo_get_link(topo, 1)) == 1234,
	       "incorrect load for link [1] 0->2");
  UTEST_ASSERT(net_iface_get_load(ez_topo_get_link(topo, 2)->dest.iface)
	       == 1234,
	       "incorrect load for link [2'] 2->1");
  ez_topo_destroy(&topo);
  return UTEST_SUCCESS;
}

// -----[ test_traffic_replay_unreach ]------------------------------
static int test_traffic_replay_unreach()
{
  ez_topo_t * topo= _ez_topo_triangle_rtr();
  flow_stats_t stats;
  ez_topo_igp_compute(topo, 1);

  flow_stats_init(&stats);
  UTEST_ASSERT(node_load_flow(ez_topo_get_node(topo, 0), IP_ADDR_ANY,
			      IPV4(192,168,1,1),
			      1234, &stats, NULL, NULL) == ESUCCESS,
	       "node_load_flow() should succeed");
  UTEST_ASSERT(stats.flows_error == 1,
	       "One flow should be reported as failure");
  ez_topo_destroy(&topo);
  return UTEST_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// MAIN PART
//
/////////////////////////////////////////////////////////////////////

#define ARRAY_SIZE(A) sizeof(A)/sizeof(A[0])

unit_test_t TEST_SIM[]= {
  {test_sim_static, "Static scheduling"},
  {test_sim_static_clear, "Static scheduling (clear)"},
  {test_sim_dynamic, "Dynamic scheduling"},
  {test_sim_dynamic_clear, "Dynamic scheduling (clear)"},
};
#define TEST_SIM_SIZE ARRAY_SIZE(TEST_SIM)

unit_test_t TEST_NET_ATTR[]= {
  {test_net_attr_address4, "IPv4 address"},
  {test_net_attr_address4_2str, "IPv4 address (-> string)"},
  {test_net_attr_address4_str2, "IPv4 address (<- string)"},
  {test_net_attr_prefix4, "IPv4 prefix"},
  {test_net_attr_prefix4_in, "IPv4 prefix in prefix"},
  {test_net_attr_prefix4_ge, "IPv4 prefix ge prefix"},
  {test_net_attr_prefix4_le, "IPv4 prefix le prefix"},
  {test_net_attr_prefix4_match, "IPv4 prefix match"},
  {test_net_attr_prefix4_mask, "IPv4 prefix mask"},
  {test_net_attr_prefix4_2str, "IPv4 prefix (-> string)"},
  {test_net_attr_prefix4_str2, "IPv4 prefix (<- string)"},
  {test_net_attr_prefix4_cmp, "IPv4 prefix (compare)"},
  {test_net_attr_dest, "IPv4 destination"},
  {test_net_attr_dest_str2, "IPv4 destination (<- string)"},
  {test_net_attr_igpweight, "IGP weight"},
  {test_net_attr_igpweight_add, "IGP weight add"},
};
#define TEST_NET_ATTR_SIZE ARRAY_SIZE(TEST_NET_ATTR)

unit_test_t TEST_NET_NODE[]= {
  {test_net_node, "node"},
  {test_net_node_0, "node (IP_ADDR_ANY)"},
  {test_net_node_name, "node name"},
};
#define TEST_NET_NODE_SIZE ARRAY_SIZE(TEST_NET_NODE)

unit_test_t TEST_NET_SUBNET[]= {
  {test_net_subnet, "subnet"},
};
#define TEST_NET_SUBNET_SIZE ARRAY_SIZE(TEST_NET_SUBNET)

unit_test_t TEST_NET_IFACE[]= {
  {test_net_iface_lo, "interface lo"},
  {test_net_iface_rtr, "interface rtr"},
  {test_net_iface_ptp, "interface ptp"},
  {test_net_iface_ptmp, "interface ptmp"},
  {test_net_iface_virtual, "interface virtual"},
  {test_net_iface_str2type, "interface (string -> type)"},
  {test_net_iface_list, "interface-list"},
  {test_net_iface_list_duplicate, "interface-list (duplicate)"},
};
#define TEST_NET_IFACE_SIZE ARRAY_SIZE(TEST_NET_IFACE)

unit_test_t TEST_NET_LINK[]= {
  {test_net_link, "link"},
  {test_net_link_forward, "link forward"},
  {test_net_link_forward_down, "link forward (down)"},
  {test_net_link_ptp, "ptp"},
  {test_net_link_ptp_forward, "ptp forward"},
  {test_net_link_ptp_forward_down, "ptp forward (down)"},
  {test_net_link_ptmp, "ptmp"},
  {test_net_link_ptmp_dup, "ptmp (dup)"},
  {test_net_link_ptmp_forward, "ptmp forward"},
  {test_net_link_ptmp_forward_unreach, "ptmp forward (unreach)"},
};
#define TEST_NET_LINK_SIZE ARRAY_SIZE(TEST_NET_LINK)

unit_test_t TEST_NET_RT[]= {
  {test_net_rt_entries, "entries"},
  {test_net_rt, "routing table"},
  {test_net_rt_add, "add"},
  {test_net_rt_add_dup, "add (dup)"},
  {test_net_rt_del, "del"},
  {test_net_rt_lookup, "lookup"},
};
#define TEST_NET_RT_SIZE ARRAY_SIZE(TEST_NET_RT)

unit_test_t TEST_NET_TUNNEL[]= {
  {test_net_tunnel, "tunnel"},
  {test_net_tunnel_forward, "tunnel forward"},
  {test_net_tunnel_recv, "tunnel recv"},
  {test_net_tunnel_src, "tunnel (src)"},
};
#define TEST_NET_TUNNEL_SIZE ARRAY_SIZE(TEST_NET_TUNNEL)

unit_test_t TEST_NET_NETWORK[]= {
  {test_net_network, "network"},
  {test_net_network_add_node, "network add node"},
  {test_net_network_add_node_dup, "network add node (duplicate)"},
  {test_net_network_add_subnet, "network add subnet"},
  {test_net_network_add_subnet_dup, "network add subnet (duplicate)"},
  {test_net_network_node_send, "node send"},
  {test_net_network_node_send_src, "node send (src-addr)"},
  {test_net_network_node_send_src_invalid, "node send (src-addr,invalid)"},
};
#define TEST_NET_NETWORK_SIZE ARRAY_SIZE(TEST_NET_NETWORK)

unit_test_t TEST_NET_RT_STATIC[]= {
  {test_net_rt_static_add, "add"},
  {test_net_rt_static_add_dup, "add (duplicate)"},
  {test_net_rt_static_remove, "remove"},
};
#define TEST_NET_RT_STATIC_SIZE ARRAY_SIZE(TEST_NET_RT_STATIC)

unit_test_t TEST_NET_RT_IGP[]= {
  {test_net_igp_domain, "igp domain"},
  {test_net_igp_domain_add, "igp domain (add)"},
  {test_net_igp_domain_add_dup, "igp domain (duplicate)"},
  {test_net_igp_compute, "igp compute"},
  {test_net_igp_compute_line_rtr, "igp compute (line rtr)"},
  {test_net_igp_compute_line_ptp, "igp compute (line ptp)"},
  {test_net_igp_compute_link_down, "igp compute (link down)"},
  {test_net_igp_compute_link_weight_0, "igp compute (link weight=0)"},
  {test_net_igp_compute_link_weight_max, "igp compute (link weight=max)"},
  {test_net_igp_compute_link_domain_boundary,
   "igp compute (domain boundary)"},
  {test_net_igp_compute_triangle_rtr, "igp compute (triangle rtr)"},
  {test_net_igp_compute_triangle_ptp, "igp compute (triangle ptp)"},
  {test_net_igp_compute_subnet, "igp compute (subnet)"},
  {test_net_igp_compute_subnet_stub, "igp compute (subnet stub)"},
  {test_net_igp_compute_loopback, "igp compute (loopback)"},
  {test_net_igp_compute_ecmp_square, "igp compute ecmp (square)"},
  {test_net_igp_compute_ecmp_complex, "igp compute ecmp (complex)"},
  {test_net_igp_ecmp3, "igp ecmp (3)"},
};
#define TEST_NET_RT_IGP_SIZE ARRAY_SIZE(TEST_NET_RT_IGP)

unit_test_t TEST_NET_TRACES[]= {
  {test_net_traces_ping, "ping"},
  {test_net_traces_ping_self_loopback, "ping (self,loopback)"},
  {test_net_traces_ping_self_iface, "ping (self,iface)"},
  {test_net_traces_ping_unreach, "ping (unreach)"},
  {test_net_traces_ping_no_reply, "ping (no reply)"},
  {test_net_traces_recordroute, "record-route"},
  {test_net_traces_recordroute_unreach, "record-route (unreach)"},
  {test_net_traces_recordroute_broken, "record-route (broken)"},
  {test_net_traces_recordroute_load, "record-route (load)"},
  {test_net_traces_recordroute_qos, "record-route (qos)"},
  {test_net_traces_recordroute_prefix, "record-route (prefix)"},
  {test_net_traces_recordroute_prefix_unreach,
   "record-route prefix (unreach)"},
  {test_net_traces_recordroute_prefix_load, "record-route prefix (load)"},
  {test_net_traces_recordroute_ecmp, "record-route ECMP"},
  {test_net_traces_recordroute_ecmp_unreach, "record-route ECMP (unreach)"},
  {test_net_traces_recordroute_ecmp_broken, "record-route ECMP (broken)"},
  {test_net_traces_recordroute_ecmp_load, "record-route ECMP (load)"},
  {test_net_traces_recordroute_tunnel, "record-route tunnel"},
  {test_net_traces_recordroute_tunnel_ecmp, "record-route tunnel+ECMP"},
  {test_net_traces_recordroute_tunnel_unreach,
   "record-route tunnel (unreach)"},
  {test_net_traces_recordroute_tunnel_load, "record-route tunnel (load)"},
  {test_net_traces_recordroute_tunnel_qos, "record-route tunnel (qos)"},
  {test_net_traces_recordroute_tunnel_prefix, "record-route tunnel (prefix)"},
};
#define TEST_NET_TRACES_SIZE ARRAY_SIZE(TEST_NET_TRACES)

unit_test_t TEST_BGP_ATTR[]= {
  {test_bgp_attr_aspath, "as-path"},
  {test_bgp_attr_aspath_prepend, "as-path prepend"},
  {test_bgp_attr_aspath_prepend_too_much, "as-path prepend (too much)"},
  {test_bgp_attr_aspath_2str, "as-path (-> string)"},
  {test_bgp_attr_aspath_str2, "as-path (<- string)"},
  {test_bgp_attr_aspath_set_str2, "as-path set (<- string)"},
  {test_bgp_attr_aspath_cmp, "as-path (compare)"},
  {test_bgp_attr_aspath_contains, "as-path (contains)"},
  {test_bgp_attr_aspath_rem_private, "as-path remove private"},
  {test_bgp_attr_aspath_match, "as-path match"},
  {test_bgp_attr_communities, "communities"},
  {test_bgp_attr_communities_append, "communities append"},
  {test_bgp_attr_communities_remove, "communities remove"},
  {test_bgp_attr_communities_2str, "communities (-> string)"},
  {test_bgp_attr_communities_str2, "communities (<- string)"},
  {test_bgp_attr_communities_cmp, "communities (compare)"},
  {test_bgp_attr_communities_contains, "communities (contains)"},
  {test_bgp_attr_cluster_list, "cluster-id-list"},
  {test_bgp_attr_cluster_list_append, "cluster-id-list append"},
  {test_bgp_attr_cluster_list_contains, "cluster-id-list contains"},
};
#define TEST_BGP_ATTR_SIZE ARRAY_SIZE(TEST_BGP_ATTR)

unit_test_t TEST_BGP_ROUTE[]= {
  {test_bgp_route_basic, "basic"},
  {test_bgp_route_communities, "attr-communities"},
  {test_bgp_route_aspath, "attr-aspath"},
};
#define TEST_BGP_ROUTE_SIZE ARRAY_SIZE(TEST_BGP_ROUTE)

unit_test_t TEST_BGP_FILTER_ACTION[]= {
  {test_bgp_filter_action_comm_add, "comm add"},
  {test_bgp_filter_action_comm_remove, "comm remove"},
  {test_bgp_filter_action_comm_strip, "comm strip"},
  {test_bgp_filter_action_local_pref, "local-pref"},
  {test_bgp_filter_action_metric, "metric"},
  {test_bgp_filter_action_metric_internal, "metric internal"},
  {test_bgp_filter_action_path_prepend, "as-path prepend"},
  {test_bgp_filter_action_path_insert, "as-path insert"},
  {test_bgp_filter_action_comm_add_str2, "comm add (string ->)"},
  {test_bgp_filter_action_comm_remove_str2, "comm remove (string ->)"},
  {test_bgp_filter_action_comm_strip_str2, "comm strip (string ->)"},
  {test_bgp_filter_action_local_pref_str2, "local-pref (string ->)"},
  {test_bgp_filter_action_metric_str2, "metric (string ->)"},
  {test_bgp_filter_action_metric_internal_str2, "metric internal (string ->)"},
  {test_bgp_filter_action_path_prepend_str2, "as-path prepend (string ->)"},
  {test_bgp_filter_action_expression_str2, "expression (string ->)"},
};
#define TEST_BGP_FILTER_ACTION_SIZE ARRAY_SIZE(TEST_BGP_FILTER_ACTION)

unit_test_t TEST_BGP_FILTER_PRED[]= {
  {test_bgp_filter_predicate_comm_contains, "comm contains"},
  {test_bgp_filter_predicate_nexthop_is, "next-hop is"},
  {test_bgp_filter_predicate_nexthop_in, "next-hop in"},
  {test_bgp_filter_predicate_prefix_is, "prefix is"},
  {test_bgp_filter_predicate_prefix_in, "prefix in"},
  {test_bgp_filter_predicate_prefix_ge, "prefix ge"},
  {test_bgp_filter_predicate_prefix_le, "prefix le"},
  {test_bgp_filter_predicate_path_regexp, "path regexp"},
  {test_bgp_filter_predicate_and, "and(x,y)"},
  {test_bgp_filter_predicate_and_any, "and(any,x)"},
  {test_bgp_filter_predicate_not, "not(x)"},
  {test_bgp_filter_predicate_not_not, "not(not(x))"},
  {test_bgp_filter_predicate_or, "or(x,y)"},
  {test_bgp_filter_predicate_or_any, "or(any,y)"},
  {test_bgp_filter_predicate_comm_contains_str2, "comm contains (string ->)"},
  {test_bgp_filter_predicate_nexthop_is_str2, "nexthop is (string ->)"},
  {test_bgp_filter_predicate_nexthop_in_str2, "nexthop in (string ->)"},
  {test_bgp_filter_predicate_prefix_is_str2, "prefix is (string ->)"},
  {test_bgp_filter_predicate_prefix_in_str2, "prefix in (string ->)"},
  {test_bgp_filter_predicate_prefix_ge_str2, "prefix ge (string ->)"},
  {test_bgp_filter_predicate_prefix_le_str2, "prefix le (string ->)"},
  {test_bgp_filter_predicate_path_regexp_str2, "path regexp (string ->)"},
  {test_bgp_filter_predicate_expr_str2, "expression (string ->)"},
};
#define TEST_BGP_FILTER_PRED_SIZE ARRAY_SIZE(TEST_BGP_FILTER_PRED)

unit_test_t TEST_BGP_ROUTE_MAPS[]= {
};
#define TEST_BGP_ROUTE_MAPS_SIZE ARRAY_SIZE(TEST_BGP_ROUTE_MAPS)

unit_test_t TEST_BGP_PEER[]= {
  {test_bgp_peer, "create"},
  {test_bgp_peer_open, "open"},
  {test_bgp_peer_open_error_unreach, "open (error, unreach)"},
  {test_bgp_peer_open_error_proto, "open (error, proto)"},
  {test_bgp_peer_close, "close"},
};
#define TEST_BGP_PEER_SIZE ARRAY_SIZE(TEST_BGP_PEER)

unit_test_t TEST_BGP_ROUTER[]= {
  {test_bgp_router, "create"},
  {test_bgp_router_no_iface, "create (error, no interface)"},
  {test_bgp_router_add_network, "add network"},
  {test_bgp_router_add_network_dup, "add network (duplicate)"},
};
#define TEST_BGP_ROUTER_SIZE ARRAY_SIZE(TEST_BGP_ROUTER)

unit_test_t TEST_BGP_DOMAIN[]= {
  {test_bgp_domain_full_mesh, "full-mesh"},
  {test_bgp_domain_full_mesh_ptp, "full-mesh (ptp)"},
};
#define TEST_BGP_DOMAIN_SIZE ARRAY_SIZE(TEST_BGP_DOMAIN)

unit_test_t TEST_MRTD[]= {
  {test_mrtd_parse, "parse"},
  {test_mrtd_parse_miss_fields, "parse (error:missing fields)"},
  {test_mrtd_parse_inv_header, "parse (error:invalid header)"},
  {test_mrtd_parse_inv_peerip, "parse (error:invalid peer ip)"},
  {test_mrtd_parse_inv_peerasn, "parse (error:invalid peer asn)"},
  {test_mrtd_parse_inv_prefix, "parse (error:invalid prefix)"},
  {test_mrtd_parse_inv_nexthop, "parse (error:invalid nexthop)"},
  {test_mrtd_parse_inv_origin, "parse (error:invalid origin)"},
};
#define TEST_MRTD_SIZE ARRAY_SIZE(TEST_MRTD)

unit_test_t TEST_CLI[]= {
  {test_cli_empty, "empty line"},
  {test_cli_comment, "comment"},
  {test_cli_error, "error"},
};
#define TEST_CLI_SIZE ARRAY_SIZE(TEST_CLI)

unit_test_t TEST_TRAFFIC[]= {
  {test_traffic_replay, "replay"},
  {test_traffic_replay_unreach, "replay (unreach)"},
};
#define TEST_TRAFFIC_SIZE ARRAY_SIZE(TEST_TRAFFIC)

unit_test_suite_t TEST_SUITES[]= {
  {"Simulator", TEST_SIM_SIZE, TEST_SIM},
  {"Net Attributes", TEST_NET_ATTR_SIZE, TEST_NET_ATTR},
  {"Net Nodes", TEST_NET_NODE_SIZE, TEST_NET_NODE},
  {"Net Subnets", TEST_NET_SUBNET_SIZE, TEST_NET_SUBNET},
  {"Net Interfaces", TEST_NET_IFACE_SIZE, TEST_NET_IFACE},
  {"Net Links", TEST_NET_LINK_SIZE, TEST_NET_LINK},
  {"Net RT", TEST_NET_RT_SIZE, TEST_NET_RT},
  {"Net Tunnels", TEST_NET_TUNNEL_SIZE, TEST_NET_TUNNEL},
  {"Net Network", TEST_NET_NETWORK_SIZE, TEST_NET_NETWORK},
  {"Net Routing Static", TEST_NET_RT_STATIC_SIZE, TEST_NET_RT_STATIC},
  {"Net Routing IGP", TEST_NET_RT_IGP_SIZE, TEST_NET_RT_IGP},
  {"Net Traces", TEST_NET_TRACES_SIZE, TEST_NET_TRACES},
  {"BGP Attributes", TEST_BGP_ATTR_SIZE, TEST_BGP_ATTR},
  {"BGP Routes", TEST_BGP_ROUTE_SIZE, TEST_BGP_ROUTE},
  {"BGP Filter Actions", TEST_BGP_FILTER_ACTION_SIZE, TEST_BGP_FILTER_ACTION},
  {"BGP Filter Predicates", TEST_BGP_FILTER_PRED_SIZE, TEST_BGP_FILTER_PRED},
  {"BGP Route Maps", TEST_BGP_ROUTE_MAPS_SIZE, TEST_BGP_ROUTE_MAPS},
  {"BGP Router", TEST_BGP_ROUTER_SIZE, TEST_BGP_ROUTER},
  {"BGP Peer", TEST_BGP_PEER_SIZE, TEST_BGP_PEER},
  {"BGP Domain", TEST_BGP_DOMAIN_SIZE, TEST_BGP_DOMAIN},
  {"MRTD", TEST_MRTD_SIZE, TEST_MRTD},
  {"CLI", TEST_CLI_SIZE, TEST_CLI},
  {"Traffic", TEST_TRAFFIC_SIZE, TEST_TRAFFIC},
};
#define TEST_SUITES_SIZE ARRAY_SIZE(TEST_SUITES)

unit_test_t SINGLE_TEST[]= {
  {test_net_traces_recordroute, "record-route"},
  {test_net_traces_recordroute_unreach, "record-route (unreach)"},
  {test_net_traces_recordroute_load, "record-route (load)"},
  {test_net_traces_recordroute_tunnel, "record-route (tunnel)"},
  {test_net_traces_recordroute_tunnel_ecmp, "record-route (tunnel,ecmp)"},
  {test_net_traces_recordroute_ecmp, "record-route (ecmp)"},
  {test_net_traces_recordroute_ecmp_load, "record-route (ecmp,load)"},
  {test_net_traces_recordroute_ecmp_unreach, "record-route (ecmp,unreach)"},
  {test_net_traces_recordroute_qos, "record-route (qos)"},
  {test_net_traces_recordroute_prefix, "record-route (prefix)"},
  {test_net_traces_recordroute_prefix_unreach,
  "record-route prefix (unreach)"},
  {test_net_traces_recordroute_prefix_load, "record-route prefix (load)"},
  {test_net_traces_recordroute_tunnel_qos, "record-route (tunnel,qos)"},
  {test_net_traces_recordroute_tunnel_unreach,
  "record-route tunnel (unreach)"},
  {test_net_traces_recordroute_tunnel_prefix, "record-route tunnel (prefix)"},
};
#define SINGLE_TEST_SIZE ARRAY_SIZE(SINGLE_TEST)

//#define USE_SINGLE_TEST

// -----[ libcbgp_selfcheck ]----------------------------------------
CBGP_EXP_DECL
int libcbgp_selfcheck()
{
  int result;

  utest_init(0);
  //utest_set_fork();
  utest_set_user(getenv("USER"));
  utest_set_project(PACKAGE_NAME, PACKAGE_VERSION);
  utest_set_xml_logging("cbgp-internal-check.xml");
#ifndef USE_SINGLE_TEST
  result= utest_run_suites(TEST_SUITES, TEST_SUITES_SIZE);
#else
  result= utest_run_suite("Single test", SINGLE_TEST,
			  SINGLE_TEST_SIZE, NULL, NULL);
#endif /* USE_SINGLE_TEST */
  utest_done();

  return result;
}
