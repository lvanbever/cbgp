// ==================================================================
// @(#)main-dump.c
//
// Main source file for cbgp-dump application.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/05/2007
// $Id: main-dump.c,v 1.7 2009-04-02 19:10:55 bqu Exp $
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/stream.h>
#include <api.h>
#include <bgp/as.h>
#include <bgp/filter/filter.h>
#include <bgp/filter/predicate_parser.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <bgp/route-input.h>

enum {
  OPTION_HELP,
  OPTION_IN_FORMAT,
  OPTION_OUT_FORMAT,
  OPTION_OUT_SPEC,
  OPTION_FILTER
} options_t;

static struct option longopts[]= {
  {"filter", required_argument, NULL, OPTION_FILTER},
  {"help", no_argument, NULL, OPTION_HELP},
  {"in-fmt", required_argument, NULL, OPTION_IN_FORMAT},
  {"out-fmt", required_argument, NULL, OPTION_OUT_FORMAT},
  {"out-spec", required_argument, NULL, OPTION_OUT_SPEC},
};

typedef struct {
  unsigned int       num_routes_ok;
  unsigned int       num_routes_ignored;
  unsigned int       num_routes_filtered;
  char             * out_spec;
  bgp_ft_matcher_t * matcher;
} dump_ctx_t;
static dump_ctx_t dump_ctx= {
  .num_routes_ok= 0,
  .num_routes_ignored= 0,
  .num_routes_filtered= 0,
  .out_spec= NULL,
  .matcher= NULL,
};

// -----[ usage ]----------------------------------------------------
void usage()
{
  printf("C-BGP route dumper %s\n", PACKAGE_VERSION);
  printf("Copyright (C) 2007, Bruno Quoitin\n"
	 "IP Networking Lab, CSE Dept, UCL, Belgium\n"
	 "\n");
  printf("Usage: cbgp-dump [OPTION]... [FILE]...\n"
	 "\n"
	 "  --filter=FILTER     specifies a route filter.\n"
	 "  --help              prints this message.\n"
	 "  --in-fmt=FORMAT     specifies the input file format. The default "
	 "format\n"
	 "                      MRT ASCII. Available file formats are:\n"
	 "                        (cisco)      CISCO's show ip bgp format\n"
	 "                        (mrt-ascii)  MRT ASCII format\n");
#ifdef HAVE_BGPDUMP
  printf("                        (mrt-binary) MRT binary\n");
#endif
  printf("  --out-fmt=FORMAT    specifies the output format. The default "
	 "format is\n"
	 "                      CISCO's show ip bgp. Available file formats "
	 "are:\n"
	 "                        (cisco)      CISCO's show ip bgp format\n"
	 "                        (mrt-ascii)  MRT ASCII format\n"
	 "                        (custom)     custom format (see "
	 "--out-spec)\n"
	 "  --out-spec=SPEC     specifies the format string for the custom "
	 "output format.\n"
	 "\n");
}

// -----[ _handler ]-------------------------------------------------
static int _handler(int status, bgp_route_t * route,
		    net_addr_t peer_addr, asn_t peer_asn,
		    void * handler_ctx)
{
  dump_ctx_t * ctx= (dump_ctx_t *) handler_ctx;
  bgp_peer_t peer;

  // Maintain statistics for discarded routes
  if (status != BGP_INPUT_STATUS_OK) {
    switch (status) {
    case BGP_INPUT_STATUS_IGNORED:
      ctx->num_routes_ignored++;
      break;
    case BGP_INPUT_STATUS_FILTERED:
      ctx->num_routes_filtered++;
      break;
    }
    return BGP_INPUT_SUCCESS;
  }

  // Filter route (if filter specified)
  if (ctx->matcher != NULL) {
    if (filter_matcher_apply(ctx->matcher, NULL, route) != 1) {
      ctx->num_routes_filtered++;
      route_destroy(&route);
      return BGP_INPUT_SUCCESS;
    }
  }

  // Create fake BGP peer
  peer.addr= peer_addr;
  peer.asn= peer_asn;
  route->peer= &peer;

  // Display route
  //log_printf(gdsout, "TABLE_DUMP|0|");
  route_dump(gdsout, route);
  stream_flush(gdsout);
  stream_printf(gdsout, "\n");
  ctx->num_routes_ok++;
  route_destroy(&route);
  return BGP_INPUT_SUCCESS;
}

// -----[ _main_done ]-----------------------------------------------
static void _main_done() __attribute((destructor));
static void _main_done()
{
  if (dump_ctx.out_spec != NULL)
    free(dump_ctx.out_spec);
  if (dump_ctx.matcher != NULL)
    filter_matcher_destroy(&dump_ctx.matcher);

  libcbgp_done();
}

// -----[ main ]-----------------------------------------------------
int main(int argc, char * argv[])
{
  bgp_input_type_t in_format= BGP_ROUTES_INPUT_MRT_ASC;
  uint8_t out_format= BGP_ROUTES_OUTPUT_CISCO;
  int option, result;
  char * filter;

  libcbgp_init(argc, argv);

  // Parse options
  while ((option= getopt_long(argc, argv, "", longopts, NULL)) != -1) {
    switch (option) {
    case OPTION_FILTER:
      filter= strdup(optarg);
      if (dump_ctx.matcher != NULL) {
	stream_printf(gdserr, "Error: route filter already specified.\n");
	return EXIT_FAILURE;
      }
      result= predicate_parser(filter, &dump_ctx.matcher);
      if (result != PREDICATE_PARSER_SUCCESS) {
	stream_printf(gdserr, "Error: invalid route filter \"%s\" [", optarg);
	predicate_parser_perror(gdserr, result);
	stream_printf(gdserr, "\n");
	return EXIT_FAILURE;
      }
      break;
    case OPTION_HELP:
      usage();
      return EXIT_SUCCESS;
    case OPTION_IN_FORMAT:
      if (bgp_routes_str2format(optarg, &in_format) != 0) {
	stream_printf(gdserr, "Error: invalid input format \"%s\".\n", optarg);
	return EXIT_FAILURE;
      }
      break;
    case OPTION_OUT_FORMAT:
      if (route_str2mode(optarg, &out_format) != 0) {
	stream_printf(gdserr, "Error: invalid output format \"%s\".\n",
		      optarg);
	return EXIT_FAILURE;
      }
      break;
    case OPTION_OUT_SPEC:
      if (dump_ctx.out_spec != NULL) {
	stream_printf(gdserr, "Error: output spec already specified.\n");
	return EXIT_FAILURE;
      }
      dump_ctx.out_spec= strdup(optarg);
      break;
    default:
      usage();
      return EXIT_FAILURE;
    }
  }

  if (optind >= argc) {
    stream_printf(gdserr, "Error: no input file.\n");
    return EXIT_FAILURE;
  }

  route_set_show_mode(out_format, dump_ctx.out_spec);

  // Parse input file(s)
  while (optind < argc) {

    stream_printf(gdserr, "Reading from \"%s\"...\n", argv[optind]);

    result= bgp_routes_load(argv[optind], in_format, _handler, &dump_ctx);
    if (result != 0) {
      stream_printf(gdserr, "Error: an error (%d) occured while "
		    "reading \"%s\"", result, argv[optind]);
      libcbgp_done();
      return EXIT_SUCCESS;
    }
    optind++;
  }

  // Print statistics
  stream_printf(gdserr, "\n");
  stream_printf(gdserr, "Summary:\n");
  stream_printf(gdserr, "  routes dumped  : %d\n",
		dump_ctx.num_routes_ok);
  stream_printf(gdserr, "  routes ignored : %d\n",
		dump_ctx.num_routes_ignored);
  stream_printf(gdserr, "  routes filtered: %d\n",
		dump_ctx.num_routes_filtered);

  return EXIT_SUCCESS;
}
