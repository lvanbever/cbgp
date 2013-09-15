// ==================================================================
// @(#)netflow.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/05/2007
// $Id: netflow.c,v 1.4 2009-08-31 09:48:28 bqu Exp $
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

#include <string.h>

#include <libgds/stream.h>
#include <libgds/str_util.h>

#include <net/netflow.h>
#include <net/prefix.h>
#include <net/util.h>

#define MAX_NETFLOW_LINE_LEN 100
static lrp_t * _parser;

// ----- Parser's states -----
typedef enum {
  NETFLOW_STATE_HEADER,
  NETFLOW_STATE_RECORDS,
} _netflow_parser_state_t;

#ifdef HAVE_LIBZ
#include <zlib.h>
typedef gzFile FILE_TYPE;
#define FILE_OPEN(N,A) gzopen(N, A)
#define FILE_CLOSE(F) gzclose(F)
#define FILE_GETS(F,B,L) gzgets(F, B, L)
#define FILE_EOF(F) gzeof(F)
#else
typedef FILE FILE_TYPE;
#define FILE_OPEN(N,A) fopen(N, A)
#define FILE_CLOSE(F) fclose(F)
#define FILE_GETS(F,B,L) fgets(B,L,F)
#define FILE_EOF(F) feof(F)
#endif

// -----[ netflow_perror ]-----------------------------------------
void netflow_perror(gds_stream_t * stream, int error)
{
  const char * error_msg= netflow_strerror(error);
  if (error_msg != NULL)
    stream_printf(stream, error_msg);
  else
    stream_printf(stream, "unknown error (%i)", error);
}

// -----[ netflow_strerror ]-----------------------------------------
const char * netflow_strerror(int error)
{
  return lrp_strerror(_parser);
  switch (error) {
  case NETFLOW_SUCCESS:
    return "success";
  case NETFLOW_ERROR_OPEN:
    return "unable to open file";
  case NETFLOW_ERROR_NUM_FIELDS:
    return "incorrect number of fields";
  case NETFLOW_ERROR_INVALID_HEADER:
    return "invalid header field";
  case NETFLOW_ERROR_INVALID_SRC_ADDR:
    return "invalid source address";
  case NETFLOW_ERROR_INVALID_DST_ADDR:
    return "invalid destination address";
  case NETFLOW_ERROR_INVALID_SRC_ASN:
    return "invalid source ASN";
  case NETFLOW_ERROR_INVALID_DST_ASN:
    return "invalid destination ASN";
  case NETFLOW_ERROR_INVALID_OCTETS:
    return "invalid octets field";
  case NETFLOW_ERROR_MISSING_FIELD:
    return "missing field";
  }
}

typedef int (*_parse_field_f)(const char * value, flow_t * flow);

static int _parse_field_src_asn(const char * value, flow_t * flow)
{
  if (str2asn(value, &flow->src_asn) < 0)
    return NETFLOW_ERROR_INVALID_SRC_ASN;
  return 0;
}

static int _parse_field_dst_asn(const char * value, flow_t * flow)
{
  if (str2asn(value, &flow->dst_asn) < 0)
    return NETFLOW_ERROR_INVALID_DST_ASN;
  return 0;
}

static int _parse_field_src_ip(const char * value, flow_t * flow)
{
  if (str2address(value, &flow->src_addr) < 0)
    return NETFLOW_ERROR_INVALID_SRC_ADDR;
  return 0;
}

static int _parse_field_dst_ip(const char * value, flow_t * flow)
{
  if (str2address(value, &flow->dst_addr) < 0)
    return NETFLOW_ERROR_INVALID_DST_ADDR;
  return 0;
}

static int _parse_field_dst_mask(const char * value, flow_t * flow)
{
  unsigned int mask;
  if ((str_as_uint(value, &mask) < 0) || (mask > 32))
    return NETFLOW_ERROR_INVALID_DST_MASK;
  flow->dst_mask= mask;
  return 0;
}

static int _parse_field_octets(const char * value, flow_t * flow)
{
  if (str_as_uint(value, &flow->bytes) < 0)
    return NETFLOW_ERROR_INVALID_OCTETS;
  return 0;
}

static _parse_field_f _parse_funcs[FLOW_FIELD_MAX]= {
  _parse_field_src_asn,  // src ASN
  _parse_field_dst_asn,  // dst ASN
  _parse_field_src_ip,   // src IP
  _parse_field_dst_ip,   // dst IP
  NULL,                  // src mask
  _parse_field_dst_mask, // dst mask
  _parse_field_octets,   // octets
  NULL,                  // packets
  NULL,                  // src port
  NULL,                  // dst port
  NULL,                  // prot
};

// -----[ _parse ]---------------------------------------------------
/**
 * Parse Netflow data produced by the flow-print utility (flow-tools)
 * with the default output format. The expected format is as follows.
 * - lines starting with '#' are ignored (comments)
 * - first non-comment line is header and must contain
 *    "srcIP  dstIP  prot  srcPort  dstPort  octets  packets"
 * - following lines contain a flow description
 *    <srcIP> <dstIP> <prot> <srcPort> <dstPort> <octets> <packets>
 *
 * For each Netflow record, the handler is called with the Netflow
 * fields (src, dst, octets) and the given context pointer.
 */
static inline 
int _parse(lrp_t * parser, flow_field_map_t * map,
	   flow_handler_f handler, void * ctx)
{
  unsigned int num_fields, num_fields_header= 0;
  int state= NETFLOW_STATE_HEADER;
  flow_field_t field;
  const char * value;
  unsigned int index;
  int result;
  flow_t flow;
  _parse_field_f func;

  while (lrp_get_next_line(parser)) {
    result= lrp_get_num_fields(parser, &num_fields);
    if (result < 0)
      return result;

    // Check header (state == 0), then records (state == 1)
    switch (state) {
    case NETFLOW_STATE_HEADER:
      num_fields_header= num_fields;
      for (index= 0; index < num_fields_header; index++) {
	field= flow_str2field(lrp_get_field(parser, index));
	if (field >= FLOW_FIELD_MAX) {
	  lrp_set_user_error(parser, "invalid header field \"%s\"",
			     lrp_get_field(parser, index));
	  return NETFLOW_ERROR_INVALID_HEADER;
	}
	if (flow_field_map_required(map, field))
	  map->index[field]= index;
      }

      // Check presence of required fields
      for (field= 0; field < FLOW_FIELD_MAX; field++) {
	if (flow_field_map_required(map, field) &&
	    !flow_field_map_isset(map, field)) {
	  lrp_set_user_error(parser, "required field missing \"%s\"",
			     flow_field2str(field));
	  return NETFLOW_ERROR_MISSING_FIELD;
	}
      }

      state= NETFLOW_STATE_RECORDS;
      break;

    case NETFLOW_STATE_RECORDS:

      // Check number of fields
      if (num_fields != num_fields_header) {
	lrp_set_user_error(parser, "incorrect number of fields (%u expected)",
			   num_fields_header);
	return NETFLOW_ERROR_NUM_FIELDS;
      }

      // Initialize flow fields
      flow.src_asn= 0;
      flow.dst_asn= 0;
      flow.bytes= 0;
      flow.src_addr= IP_ADDR_ANY;
      flow.dst_addr= IP_ADDR_ANY;
      flow.dst_mask= 32;

      // Get flow fields from file record
      for (field= 0; field < FLOW_FIELD_MAX; field++) {
	if (flow_field_map_isset(map, field)) {
	  func= _parse_funcs[field];
	  value= lrp_get_field(parser, map->index[field]);
	  if (func != NULL) {
	    result= func(value, &flow);
	    if (result < 0) {
	      lrp_set_user_error(parser, "%s (%s)", netflow_strerror(result),
				 value);
	      return result;
	    }
	  }
	}
      }

      // Process flow
      result= handler(&flow, map, ctx);
      if (result != 0)
	return NETFLOW_ERROR;
      break;

    default:
      cbgp_fatal("incorrect state number %d", state);
    }
  }
  return NETFLOW_SUCCESS;
}

// -----[ netflow_parser ]-------------------------------------------
int netflow_parser(FILE * stream, flow_field_map_t * map,
		   flow_handler_f handler, void * ctx)
{
  lrp_set_stream(_parser, stream);
  return _parse(_parser, map, handler, ctx);
}

// -----[ netflow_load ]---------------------------------------------
int netflow_load(const char * filename, flow_field_map_t * map,
		 flow_handler_f handler, void * ctx)
{
  int result= lrp_open(_parser, filename);
  if (result < 0)
    return result;
  result= _parse(_parser, map, handler, ctx);
  lrp_close(_parser);
  return result;
}

// -----[ _netflow_init ]--------------------------------------------
void _netflow_init()
{
  _parser= lrp_create(MAX_NETFLOW_LINE_LEN, " \t");
}

// -----[ _netflow_done ]--------------------------------------------
void _netflow_done()
{
  lrp_destroy(&_parser);
}
