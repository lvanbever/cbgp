// ==================================================================
// @(#)route-input.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/05/2007
// $Id: route-input.c,v 1.6 2009-08-31 09:34:39 bqu Exp $
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

#define _GNU_SOURCE

#include <string.h>

#include <bgp/cisco.h>
#include <bgp/mrtd.h>
#include <bgp/route-input.h>
#include <bgp/routes_list.h>

static char * INPUT_TYPE_STR[BGP_ROUTES_INPUT_MAX]=
{
  "mrt-ascii",
#ifdef HAVE_BGPDUMP
  "mrt-binary",
#endif /* HAVE_BGPDUMP */
  "cisco"
};

// -----[ bgp_routes_str2format ]------------------------------------
int bgp_routes_str2format(const char * format_str,
			  bgp_input_type_t * format)
{
  unsigned int index;

  for (index= 0; index < BGP_ROUTES_INPUT_MAX; index++) {
    if (!strcmp(format_str, INPUT_TYPE_STR[index])) {
      *format= index;
      return 0;
    }
  }
  return -1;
}

// -----[ bgp_routes_load ]------------------------------------------
int bgp_routes_load(const char * filename, bgp_input_type_t format,
		    bgp_route_handler_f handler, void * ctx)
{
  switch (format) {
  case BGP_ROUTES_INPUT_MRT_ASC:
    return mrtd_ascii_load(filename, handler, ctx);
#ifdef HAVE_BGPDUMP
  case BGP_ROUTES_INPUT_MRT_BIN:
    return mrtd_binary_load(filename, handler, ctx);
#endif
  case BGP_ROUTES_INPUT_CISCO:
    cbgp_fatal("cisco input format is not supported");
    return -1;//cisco_load(filename, handler, ctx);
  default:
    cbgp_fatal("invalid input format");
  }
  return -1;
}

// -----[ _bgp_routes_load_list_handler ]----------------------------
static int _bgp_routes_load_list_handler(int status,
					 bgp_route_t * route,
					 net_addr_t peer_addr,
					 asn_t peer_asn,
					 void * ctx)
{
  bgp_routes_t * routes= (bgp_routes_t *) ctx;

  if (status == BGP_INPUT_STATUS_OK)
    routes_list_append(routes, route);

  return BGP_INPUT_SUCCESS;
}

// -----[ bgp_routes_load_list ]-------------------------------------
/**
 *
 */
bgp_routes_t * bgp_routes_load_list(const char * filename,
				    bgp_input_type_t format)
{
  bgp_routes_t * routes= routes_list_create(ROUTES_LIST_OPTION_REF);
  int result;

  result= bgp_routes_load(filename, format, _bgp_routes_load_list_handler,
			  routes);  
  if (result != 0) {
    /* TODO: We should destroy all routes here !!! */
    routes_list_destroy(&routes);
    return NULL;
  }

  return routes;
}

static char * user_error= NULL;

// -----[ bgp_input_strerror ]---------------------------------------
const char * bgp_input_strerror(bgp_input_error_t error)
{
  switch (error) {
  case BGP_INPUT_SUCCESS:
    return "success";
  case BGP_INPUT_ERROR_UNEXPECTED:
    return "unexpected";
  case BGP_INPUT_ERROR_FILE_OPEN:
    return "file could not be opened";
  case BGP_INPUT_ERROR_IGNORED:
    return "route ignored";
  case BGP_INPUT_ERROR_FILTERED:
    return "route filtered";
  case BGP_INPUT_ERROR_USER:
    return user_error;
  }
  return NULL;
}

// -----[ bgp_input_perror ]-----------------------------------------
void bgp_input_perror(gds_stream_t * stream,
		      bgp_input_error_t error)
{
  const char * msg= bgp_input_strerror(error);
  if (msg != NULL)
    stream_printf(stream, "%s", msg);
  else
    stream_printf(stream, "unknown error code (%d)", error);
}

// -----[ bgp_input_set_user_error ]---------------------------------
void bgp_input_set_user_error(const char * format, ...)
{
  va_list ap;

  if (user_error != NULL)
    free(user_error);

  va_start(ap, format);
  assert(vasprintf(&user_error, format, ap) >= 0);
  assert(user_error != NULL);
  va_end(ap);
}
