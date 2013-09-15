// ==================================================================
// @(#)mrtd.c
//
// Interface with MRT data (ASCII and binary). The import of
// binary MRT data is based on Dan Ardelean's library (libbgpdump)
// now maintained by RIPE.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Dan Ardelean (dan@ripe.net, dardelea@cs.purdue.edu)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 20/02/2004
// $Id: mrtd.c,v 1.28 2009-06-25 14:26:46 bqu Exp $
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <libgds/tokenizer.h>
#include <libgds/tokens.h>

#include <net/network.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/protocol.h>
#include <net/util.h>

#include <bgp/as.h>
#include <bgp/filter/filter.h>
#include <bgp/peer.h>
#include <bgp/attr/comm.h>
#include <bgp/attr/origin.h>
#include <bgp/attr/path.h>
#include <bgp/attr/path_segment.h>
#include <bgp/mrtd.h>
#include <bgp/route.h>
#include <bgp/routes_list.h>

//#define DEBUG
#include <libgds/debug.h>

#define MRT_MAX_LINE_LEN 1024

#ifdef HAVE_LIBZ
# include <zlib.h>
typedef gzFile FILE_TYPE;
# define FILE_OPEN(N,A) gzopen(N, A)
# define FILE_DOPEN(N,A) gzdopen(N,A)
# define FILE_CLOSE(F) gzclose(F)
# define FILE_GETS(F,B,L) gzgets(F, B, L)
# define FILE_EOF(F) gzeof(F)
#else
typedef FILE * FILE_TYPE;
# define FILE_OPEN(N,A) fopen(N, A)
# define FILE_DOPEN(N,A) fdopen(N,A)
# define FILE_CLOSE(F) fclose(F)
# define FILE_GETS(F,B,L) fgets(B,L,F)
# define FILE_EOF(F) feof(F)
#endif

#define MRT_UPDATE_MIN_FIELDS   11
#define MRT_WITHDRAW_MIN_FIELDS 5

// ----- Local tokenizers -----
static gds_tokenizer_t * line_tokenizer= NULL;



// -----[ mrtd_strerror ]------------------------------------------
const char * mrtd_strerror(int error)
{
  switch (error) {
  case MRTD_ERROR:
    return "error";
  case MRTD_INVALID_PEER_ADDR:
    return "invalid peer IP";
  case MRTD_INVALID_PEER_ASN:
    return "invalid peer ASN";
  case MRTD_INVALID_ORIGIN:
    return "invalid origin";
  case MRTD_INVALID_ASPATH:
    return "invalid AS-path";
  case MRTD_INVALID_LOCALPREF:
    return "invalid local-pref";
  case MRTD_INVALID_MED:
    return "invalid MED";
  case MRTD_INVALID_COMMUNITIES:
    return "invalid communities";
  case MRTD_INVALID_RECORD_TYPE:
    return "invalid record type";
  case MRTD_INVALID_PREFIX:
    return "invalid prefix";
  case MRTD_INVALID_NEXTHOP:
    return "invalid nexthop";
  case MRTD_INVALID_PROTOCOL:
    return "invalid protocol";
  case MRTD_MISSING_FIELDS:
    return "not enough fields";
  default:
    return NULL;
  }
  return NULL;
}

static char * _user_error= NULL;
static unsigned int _line_number;
static inline void _set_user_error(const char * format, ...)
{
  va_list ap;
  
  if (_user_error != NULL)
    free(_user_error);

  va_start(ap, format);
  assert(vasprintf(&_user_error, format, ap) >= 0);
  va_end(ap);
}

// -----[ mrtd_perror ]----------------------------------------------
void mrtd_perror(gds_stream_t * stream, int error)
{
  const char * error_str;
  if (error <= MRTD_ERROR) {
    error_str= mrtd_strerror(error);
  } else {
    error_str= _user_error;
  }
  if (error_str != NULL)
    stream_printf(stream, error_str);
  else
    stream_printf(stream, "unknown error (%i)", error);
  if (_line_number > 0)
    stream_printf(stream, ", at line %u", _line_number);
}


/////////////////////////////////////////////////////////////////////
//
// ASCII MRT FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ _mrtd_check_type ]-----------------------------------------
/**
 * This function checks that the given MRT protocol field is
 * compatible with the given MRT input type.
 *
 * For table dumps (MRTD_TYPE_RIB), the protocol field is expected to
 * contain 'TABLE_DUMP'.
 *
 * For messages (MRTD_TYPE_MSG), the protocol field is expected to
 * contain 'BGP' or 'BGP4'.
 */
static inline int _mrtd_check_type(const char * field, mrtd_input_t type)
{
  return (((type == MRTD_TYPE_RIB) &&
	   !strcmp(field, "TABLE_DUMP")) ||
	  ((type != MRTD_TYPE_RIB) &&
	   (!strcmp(field, "BGP") || !strcmp(field, "BGP4")))
	  );
}

// -----[ _mrtd_check_header ]---------------------------------------
/**
 * Check an MRT route record's header. Return the MRT record type
 * which is one of 'A' (update message), 'W' (withdraw message) and
 * 'B' (best route).
 */
static inline int _mrtd_check_header(const gds_tokens_t * tokens,
				     mrtd_input_t * type_ref)
{
  unsigned int num_tokens, req_tokens;
  mrtd_input_t type;
  const char * token;

  num_tokens= tokens_get_num(tokens);

  // Check the number of fields (at least 5 for the header)
  if (num_tokens < MRT_WITHDRAW_MIN_FIELDS) {
    _set_user_error("not enough fields in MRT input (%d/5)", num_tokens);
    return MRTD_MISSING_FIELDS;
  }

  // Check the type field. This field must contain:
  //   - B for best routes in table dumps
  //   - A for updates in messages
  //   - W for withdraws in messages
  token= tokens_get_string_at(tokens, 2);
  if ((strlen(token) != 1) || (strchr("ABW", token[0]) == NULL)) {
    _set_user_error("invalid MRT record type field \"%s\"", token);
    return MRTD_INVALID_RECORD_TYPE;
  }
  type= token[0];

  // Check the protocol field. This field must contain:
  //   - TABLE_DUMP for table dumps
  //   - BGP / BGP4 for messages
  token= tokens_get_string_at(tokens, 0);
  if (!_mrtd_check_type(token, type)) {
    _set_user_error("incorrect MRT record protocol \"%s\"", token);
    return MRTD_INVALID_PROTOCOL;
  }
  
  // Check the timestamp field
  // TO BE DONE (?)

  // Check the number of fields (again)
  if (type == 'W')
    req_tokens= MRT_WITHDRAW_MIN_FIELDS;
  else
    req_tokens= MRT_UPDATE_MIN_FIELDS;

  if (num_tokens < req_tokens) {
    _set_user_error("not enough fields in MRT input (%d/%d)",
		    num_tokens, req_tokens);
    return MRTD_MISSING_FIELDS;
  }

  *type_ref= type;
  return MRTD_SUCCESS;
}

// -----[ _mrtd_create_route ]---------------------------------------
/*
 * This function builds a route from the given set of tokens. The
 * function requires at least MRT_UPDATE_MIN_FIELDS (11) tokens for a
 * route that belongs to a routing table dump or an update message
 * (communities are optional). The function requires 6 tokens for a
 * withdraw message. 
 * The set of tokens must be composed of the following fields (see the
 * MRT user's manual for more information):
 *
 * Token 0     -> Protocol
 *       1     -> Time (currently ignored)
 *       2     -> Type
 *       3     -> PeerIP
 *       4     -> PeerAS
 *       5     -> Prefix
 *       6     -> AS-Path
 *       7     -> Origin
 *       8     -> NextHop (all fields mandatory up to next-hop)
 *       9     -> Local_Pref
 *       10    -> MED
 *       11    -> Community
 *       >= 12 -> currently ignored
 *
 * Parameters:
 *   - the router which is supposed to receive this route (or NULL)
 *   - the MRT record tokens
 *   - the destination prefix
 *   - a pointer to the resulting route
 *
 * Return values:
 *    0 in case of success (and ppRoute points to a valid BGP route)
 *   <0 in case of failure
 *
 * Note: the router field must be specified (i.e. be != NULL) when the
 * MRT record contains a BGP message. In this case, the peer
 * information (Peer IP and Peer AS) contained in the MRT record must
 * correspond to the router address/AS-number and the next-hops must
 * correspond to peers of the router.
 *
 * If no BGP router is specified, the route is considered as locally
 * originated.
 */
static int _mrtd_create_route(const char * line, ip_pfx_t * prefix,
			      net_addr_t * peer_addr_ref,
			      asn_t * peer_asn_ref,
			      bgp_route_t ** route_ref)
{
  mrtd_input_t type;
  const gds_tokens_t * tokens;
  bgp_origin_t origin;
  net_addr_t next_hop;
  unsigned long pref;
  unsigned long med;
  const char * token;
  bgp_path_t * path= NULL;
  bgp_comms_t * comm= NULL;
  int result;
  net_addr_t peer_addr;
  asn_t peer_asn;

  if (line_tokenizer == NULL) {
    line_tokenizer= tokenizer_create("|", NULL, NULL);
    tokenizer_set_flag(line_tokenizer, TOKENIZER_OPT_SINGLE_DELIM);
  }

  // Parse the current line
  result= tokenizer_run(line_tokenizer, line);
  if (result != TOKENIZER_SUCCESS) {
    _set_user_error("unable to parse line (%s)", tokenizer_strerror(result));
    return MRTD_ERROR_SYNTAX;
  }
      
  tokens= tokenizer_get_tokens(line_tokenizer);

  // Check header, length and get type (route/update/withdraw)
  result= _mrtd_check_header(tokens, &type);
  if (result != MRTD_SUCCESS)
    return result;

  // Check the peer IP address field
  token= tokens_get_string_at(tokens, 3);
  if (str2address(token, &peer_addr)) {
    _set_user_error("invalid peer IP address \"%s\"", token);
    return MRTD_INVALID_PEER_ADDR;
  }
  
  // Check the peer ASN field
  token= tokens_get_string_at(tokens, 4);
  if (str2asn(token, &peer_asn) < 0) {
    _set_user_error("invalid peer ASN \"%s\"", token);
    return MRTD_INVALID_PEER_ASN;
  }

  // Check the prefix
  token= tokens_get_string_at(tokens, 5);
  if (str2prefix(token, prefix)) {
    _set_user_error("invalid prefix \"%s\"", token);
    return MRTD_INVALID_PREFIX;
  }

  if (type == MRTD_TYPE_WITHDRAW)
    return type;

  // Check the AS-PATH
  token= tokens_get_string_at(tokens, 6);
  path= path_from_string(token);
  if (path == NULL) {
    _set_user_error("invalid AS-Path \"%s\"", token);
    return MRTD_INVALID_ASPATH;
  }

  // Check ORIGIN
  token= tokens_get_string_at(tokens, 7);
  if (bgp_origin_from_str(token, &origin) < 0) {
    _set_user_error("invalid origin \"%s\"", token);
    path_destroy(&path);
    return MRTD_INVALID_ORIGIN;
  }

  // Check the NEXT-HOP
  token= tokens_get_string_at(tokens, 8);
  if (str2address(token, &next_hop)) {
    _set_user_error("invalid next-hop \"%s\"", token);
    path_destroy(&path);
    return MRTD_INVALID_NEXTHOP;
  }

  // Check the LOCAL-PREF
  token= tokens_get_string_at(tokens, 9);
  if (*token != '\0') {
    if (str_as_ulong(token, &pref) < 0) {
      _set_user_error("invalid local-preference \"%s\"", token);
      path_destroy(&path);
      return MRTD_INVALID_LOCALPREF;
    }
  } else
    pref= 0;

  // Check the MED
  token= tokens_get_string_at(tokens, 10);
  if (*token != '\0') {
    if (str_as_ulong(token, &med) < 0) {
      _set_user_error("invalid multi-exit-discriminator \"%s\"\n",
		      tokens_get_string_at(tokens, 10));
      path_destroy(&path);
      return MRTD_INVALID_MED;
    }
  } else
    med= ROUTE_MED_MISSING;

  // Check the COMMUNITIES (if present)
  if (tokens_get_num(tokens) > MRT_UPDATE_MIN_FIELDS) {
    token= tokens_get_string_at(tokens, 11);
    comm= comm_from_string(token);
    if (comm == NULL) {
      _set_user_error("invalid communities \"%s\"", token);
      path_destroy(&path);
      return MRTD_INVALID_COMMUNITIES;
    }
  }

  // Build route
  if (route_ref != NULL) {
    *route_ref= route_create(*prefix, NULL, next_hop, origin);
    route_localpref_set(*route_ref, pref);
    route_med_set(*route_ref, med);
    route_set_path(*route_ref, path);
    route_set_comm(*route_ref, comm);
    route_flag_set(*route_ref, ROUTE_FLAG_BEST, 1);
    route_flag_set(*route_ref, ROUTE_FLAG_ELIGIBLE, 1);
    route_flag_set(*route_ref, ROUTE_FLAG_FEASIBLE, 1);
  }

  if (peer_addr_ref != NULL)
    *peer_addr_ref= peer_addr;
  if (peer_asn_ref != NULL)
    *peer_asn_ref= peer_asn;

  __debug("route created from MRT record.\n");
  
  return type;
}

// -----[ mrtd_route_from_line ]-------------------------------------
/**
 * This function parses a string that contains an MRT record. Based on
 * the record's content, a BGP route is created. For more information
 * on the MRT record format, see 'mrtd_create_route'.
 *
 * Parameters:
 *
 */
int mrtd_route_from_line(const char * line,
			 net_addr_t * peer_addr_ref,
			 asn_t * peer_asn_ref,
			 bgp_route_t ** route_ref)
{
  bgp_route_t * route= NULL;
  ip_pfx_t prefix;
  int result;

  result= _mrtd_create_route(line, &prefix, peer_addr_ref, peer_asn_ref,
			     &route);
  if (result < 0) {
    __debug("could not create route from MRT record\n"
	    "  +-- line  :\"%s\"\n"
	    "  +-- result:%d (%s)\n",
	    line, result, mrtd_strerror(result));
    return result;
  }

  if (result != MRTD_TYPE_RIB) {
    route_destroy(&route);
  }

  if (route_ref != NULL) {
    *route_ref= route;
    route= NULL;
  }
  
  return result;
}

// ----- mrtd_msg_from_line -----------------------------------------
/**
 * This function parses a string that contains an MRT record. Based on
 * the record's content, a BGP Update or Withdraw message is built.
 *
 * Parameters:
 *   - the router which is supposed to receive the message (or NULL)
 *   - the peer which is supposed to send the message
 *   - the strings that contains the MRT BGP message
 *
 * Note: in case a router has been specified (i.e. pRouter != NULL),
 * the function checks that the message comes from a valid peer of the
 * router.
 */
int mrtd_msg_from_line(bgp_router_t * router, bgp_peer_t * peer,
		       const char * line, bgp_msg_t ** msg_ref)
{
  bgp_msg_t * msg= NULL;
  bgp_route_t * route= NULL;
  ip_pfx_t prefix;
  net_addr_t peer_addr;
  asn_t peer_asn;
  int result;
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  net_addr_t nexthop;
#endif

  result= _mrtd_create_route(line, &prefix, &peer_addr, &peer_asn, &route);
  if (result < 0)
    return result;

  switch (result) {
  case MRTD_TYPE_UPDATE:
    msg= bgp_msg_update_create(peer->asn, route);
    break;
  case MRTD_TYPE_WITHDRAW:
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
    //TODO: Is it possible to change the MRT format to adapt to Walton ?
    nexthop = route_get_nexthop(route);
    msg= bgp_msg_withdraw_create(peer->addr, prefix, &nexthop);
#else
    msg= bgp_msg_withdraw_create(peer->addr, prefix);
#endif
    break;
  default:
    route_destroy(&route);
  }
  
  if (msg_ref != NULL) {
    *msg_ref= msg;
    msg= NULL;
  }

  if (msg != NULL)
    bgp_msg_destroy(&msg);

  return MRTD_SUCCESS;
}

// -----[ mrtd_ascii_load ]------------------------------------------
/**
 * This function loads all the routes from a table dump in MRT
 * format. The filename must have previously been converted to ASCII
 * using 'route_btoa -m'.
 */
int mrtd_ascii_load(const char * filename, bgp_route_handler_f handler,
		    void * ctx)
{
  FILE_TYPE file;
  char line[MRT_MAX_LINE_LEN];
  bgp_route_t * route;
  int error= BGP_INPUT_SUCCESS;
  net_addr_t peer_addr;
  asn_t peer_asn;
  int result;
  int status;

  _line_number= 0;

  // Open input file
  if ((filename == NULL) || !strcmp(filename, "-")) {
    file= FILE_DOPEN(0, "r");
  } else {
    file= FILE_OPEN(filename, "r");
  }
  if (file == NULL)
    return BGP_INPUT_ERROR_FILE_OPEN;

  while (!FILE_EOF(file)) {
    if (FILE_GETS(file, line, sizeof(line)) == NULL)
      break;

    _line_number++;

    // Create a route from the file line
    result= mrtd_route_from_line(line, &peer_addr, &peer_asn, &route);

    // In case of error, the MRT record is ignored
    if (result < 0) {
      error= BGP_INPUT_ERROR_USER;
      bgp_input_set_user_error("syntax error at line %d (%s)",
			       _line_number, mrtd_strerror(result));
      break;
    }

    status= BGP_INPUT_STATUS_OK;
    if (handler(status, route, peer_addr, peer_asn, ctx) != 0) {
      error= BGP_INPUT_ERROR_UNEXPECTED;
      break;
    }
    
  }
  
  FILE_CLOSE(file);

  return error;
}


/////////////////////////////////////////////////////////////////////
//
// BINARY MRT FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

#ifdef HAVE_BGPDUMP
# include <sys/types.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <external/bgpdump_lib.h>
# include <external/bgpdump_formats.h>
# include <netinet/in.h>
#endif

// -----[ mrtd_process_community ]-----------------------------------
/**
 *
 */
#ifdef HAVE_BGPDUMP
bgp_comms_t * mrtd_process_community(struct community * com)
{
  bgp_comms_t * comms= NULL;
  unsigned int index;
  uint32_t comval;

  for (index= 0; index < com->size; index++) {
    if (comms == NULL)
      comms= comms_create();
    memcpy(&comval, com_nthval(com, index), sizeof (uint32_t));
    comms_add(&comms, ntohl(comval));
  }
  return comms;
}
#endif

// -----[ mrtd_process_aspath ]--------------------------------------
/**
 * Convert a bgpdump aspath to a C-BGP as-path. Note: this function is
 * based on Dan Ardelean's code to convert an aspath to a string.
 *
 * This function only supports SETs and SEQUENCEs. If such a segment
 * is found, the function will fail (and return NULL).
 */
#ifdef HAVE_BGPDUMP
bgp_path_t * mrtd_process_aspath(const struct aspath * path)
{
  bgp_path_t * cbgp_path= NULL;
  bgp_path_seg_t * seg;
  void * pnt;
  void * end;
  int asn_pos;
  struct assegment *assegment;
  unsigned int index;
  int result= 0;

  /* empty AS-path */
  if (path->length == 0)
    return NULL;

  /* ASN size == 32 bits (not supported by c-bgp) */
  if (path->asn_len != ASN16_LEN) {
    STREAM_ERR(STREAM_LEVEL_SEVERE, "32-bits ASN are not supported.\n");
    return NULL;
  }

  /* Set initial pointers */
  pnt= path->data;
  end= pnt + path->length;

  while (pnt < end) {
    
    /* Process next segment... */
    assegment= (struct assegment *) pnt;
    
    /* Get the AS-path segment type. */
    if (assegment->type == AS_SET) {
      seg= path_segment_create(AS_PATH_SEGMENT_SET, assegment->length);
    } else if (assegment->type == AS_SEQUENCE) {
      seg= path_segment_create(AS_PATH_SEGMENT_SEQUENCE,
			       assegment->length);
    } else {
      result= -1;
      break;
    }
    
    /* Check the AS-path segment length. */
    if ((pnt + (assegment->length * path->asn_len) + AS_HEADER_SIZE) > end) {
      result= -1;
      break;
    }

    /* Copy each AS number into the AS-path segment */
    for (index= 0; index < assegment->length; index++) {
      asn_pos = index * path->asn_len;
      seg->asns[assegment->length-index-1]=
	ntohs(*(u_int16_t *) (assegment->data + asn_pos));
    }

    /* Add the segment to the AS-path */
    if (cbgp_path == NULL)
      cbgp_path= path_create();
    path_add_segment(cbgp_path, seg);

    /* Go to next segment... */
    pnt+= (assegment->length * path->asn_len) + AS_HEADER_SIZE;
  }

  /* If something went wrong, clean current AS-path */
  if ((result != 0) && (cbgp_path != NULL)) {
    path_destroy(&cbgp_path);
    cbgp_path= NULL;
  }

  return cbgp_path;
}
#endif

// -----[ mrtd_process_table_dump ]----------------------------------
/**
 * Convert an MRT TABLE DUMP to a C-BGP route.
 *
 * This function currently only supports IPv4 address familly. In
 * addition, the function only retrive the following attributes:
 * next-hop, origin, local-pref, med, as-path
 *
 * todo: conversion of communities
 */
#ifdef HAVE_BGPDUMP
bgp_route_t * mrtd_process_table_dump(BGPDUMP_ENTRY * entry)
{
  BGPDUMP_MRTD_TABLE_DUMP * table_dump= &entry->body.mrtd_table_dump;
  bgp_route_t * route= NULL;
  ip_pfx_t prefix;
  bgp_origin_t origin;
  net_addr_t next_hop;

  if (entry->subtype == AFI_IP) {

    prefix.network= ntohl(table_dump->prefix.v4_addr.s_addr);
    prefix.mask= table_dump->mask;
    if (entry->attr == NULL)
      return NULL;

    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ORIGIN)) != 0)
      origin= (bgp_origin_t) entry->attr->origin;
    else
      return NULL;

    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_NEXT_HOP)) != 0)
      next_hop= ntohl(entry->attr->nexthop.s_addr);
    else
      return NULL;

    route= route_create(prefix, NULL, next_hop, origin);

    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AS_PATH)) != 0)
      route_set_path(route, mrtd_process_aspath(entry->attr->aspath));

    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) != 0)
      route_localpref_set(route, entry->attr->local_pref);

    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) != 0)
      route_med_set(route, entry->attr->med);

    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0)
      route_set_comm(route, mrtd_process_community(entry->attr->community));

  }

  return route;
}
#endif

// -----[ mrtd_process_entry ]---------------------------------------
/**
 * Convert a bgpdump entry to a route. Only supports entries of type
 * TABLE DUMP, for adress family AF_IP (IPv4).
 */
#ifdef HAVE_BGPDUMP
bgp_route_t * mrtd_process_entry(BGPDUMP_ENTRY * entry,
				 net_addr_t * peer_addr_ref,
				 unsigned int * peer_asn_ref)
{
  bgp_route_t * route= NULL;

  if (entry->type == BGPDUMP_TYPE_MRTD_TABLE_DUMP) {
    route= mrtd_process_table_dump(entry);
    *peer_addr_ref= ntohl(entry->body.mrtd_table_dump.peer_ip.v4_addr.s_addr);
    *peer_asn_ref= entry->body.mrtd_table_dump.peer_as;
  } else {
    STREAM_ERR(STREAM_LEVEL_SEVERE, "Error: mrtd input contains an unsupported entry.\n");
  }
  return route;
}
#endif

// -----[ mrtd_binary_load ]-----------------------------------------
/**
 *
 */
#ifdef HAVE_BGPDUMP
int mrtd_binary_load(const char * filename, bgp_route_handler_f handler,
		     void * ctx)
{
  BGPDUMP * dump;
  BGPDUMP_ENTRY * entry= NULL;
  bgp_route_t * route;
  int error= BGP_INPUT_SUCCESS;
  int status;
  net_addr_t peer_addr;
  unsigned int peer_asn;

  if ((dump= bgpdump_open_dump((char *) filename)) == NULL)
    return BGP_INPUT_ERROR_FILE_OPEN;

  do {

    peer_addr= IP_ADDR_ANY;
    peer_asn= 0;
    route= NULL;

    entry= bgpdump_read_next(dump);
    if (entry == NULL) {
      if (dump->parsed != dump->parsed_ok) {
	error= BGP_INPUT_ERROR_USER;
	bgp_input_set_user_error("malformed binary MRT entry");
	break;
      }
      continue;
    }

    route= mrtd_process_entry(entry, &peer_addr, &peer_asn);
    bgpdump_free_mem(entry);

    if (route == NULL)
      status= BGP_INPUT_STATUS_IGNORED;
    else
      status= BGP_INPUT_STATUS_OK;
      
    if (handler(status, route, peer_addr, peer_asn, ctx) != 0) {
      error= BGP_INPUT_ERROR_UNEXPECTED;
      break;
    }
      
  } while (dump->eof == 0);
  
  bgpdump_close_dump(dump);

  return error;
}
#endif


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _mrtd_destroy ]--------------------------------------------
void _mrtd_destroy()
{
  tokenizer_destroy(&line_tokenizer);
  if (_user_error != NULL)
    free(_user_error);
}
