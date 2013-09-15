// ==================================================================
// @(#)types.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/09/2008
// $Id: types.h,v 1.1 2009-03-24 13:41:26 bqu Exp $
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

/**
 * \file
 * Provide the main BGP attribute data structures.
 */

#ifndef __BGP_ATTR_TYPES_H__
#define __BGP_ATTR_TYPES_H__

#include <libgds/types.h>
#include <libgds/array.h>

#include <net/prefix.h>

/////////////////////////////////////////////////////////////////////
// Origin
/////////////////////////////////////////////////////////////////////

// -----[ bgp_origin_t ]---------------------------------------------
/** Possible BGP route origins. */
typedef enum {
  /** Static route (originated). */
  BGP_ORIGIN_IGP       = 0,
  /** Route learned through BGP. */
  BGP_ORIGIN_EGP       = 1,
  /** Route learned through redistribution. */
  BGP_ORIGIN_INCOMPLETE= 2,
  BGP_ORIGIN_MAX,
} bgp_origin_t;


/*///////////////////////////////////////////////////////////////////
 *
 * AS-Paths
 *
 */////////////////////////////////////////////////////////////////*/

 /** AS-Path segment of type SET. */
#define AS_PATH_SEGMENT_SET             1
 /** AS-Path segment of type SEQUENCE. */
#define AS_PATH_SEGMENT_SEQUENCE        2
 /** AS-Path segment of type CONFEDERATION SET. */
#define AS_PATH_SEGMENT_CONFED_SET      3
 /** AS-Path segment of type CONFEDERATION SEQUENCE. */
#define AS_PATH_SEGMENT_CONFED_SEQUENCE 4


  // -----[ bgp_path_seg_t ]-------------------------------------------
  /** Definition of an AS-Path segment. */
typedef struct {
  /** Segment type. */
  uint8_t  type;
  /** Number of ASNs in the segment. */
  uint8_t  length;
  /** Set of ASNs. */
  uint16_t asns[0];
} bgp_path_seg_t;


// -----[ bgp_path_t ]-----------------------------------------------
/** 
 * Definition of an AS-Path.
 *
 * There are two types of AS-Paths supported so far. The first one is
 * the simplest and contains the list of segments composing the
 * path. The second type can be used to reduce the memory consumption
 * and is composed of a segment and a previous AS-Path (called the
 * "tail") of the path.
 */
#ifndef __BGP_PATH_TYPE_TREE__
typedef ptr_array_t bgp_path_t;
#else
typedef struct bgp_path_t {
  bgp_attr_refcnt_t tRefCnt;
  bgp_path_segment_t * pSegment;
  struct bgp_path_t * pPrevious;
} bgp_path_t;
#endif


/////////////////////////////////////////////////////////////////////
// Communities
/////////////////////////////////////////////////////////////////////

/** Standard community: NO EXPORT. */
#define COMM_NO_EXPORT 0xffffff01
/** Standard community: NO ADVERTISE. */
#define COMM_NO_ADVERTISE 0xffffff02
/** Standard community: NO EXPORT SUBCONFED. */
#define COMM_NO_EXPORT_SUBCONFED 0xffffff03
#ifdef __EXPERIMENTAL__
#define COMM_DEPREF 0xfffffff0
#endif

#define COMM_DUMP_RAW  0
#define COMM_DUMP_TEXT 1

// -----[ bgp_comm_t ]-----------------------------------------------
/** Definition of a value in a Communities attribute (RFC1997). */
typedef uint32_t bgp_comm_t;

// -----[ bgp_comms_t ]----------------------------------------------
/** Definition of a Communities attribute (RFC1997). */
typedef struct bgp_comms_t {
  /** Number of values. */
  unsigned char num;
  /** Values. */
  bgp_comm_t    values[0];
} bgp_comms_t;


#define ECOMM_RED 3
#ifdef __EXPERIMENTAL__
#define ECOMM_PREF 4
#endif


// -----[ ecomm_red_action_type_t ]----------------------------------
/** BGP Redistribution Communities: action types.
 *  (draft-ietf-grow-bgp-redistribution-00). */
typedef enum {
  /** Prepend. */
  ECOMM_RED_ACTION_PREPEND  = 0,
  /** Attach NO EXPORT community. */
  ECOMM_RED_ACTION_NO_EXPORT= 1,
  /** Ignore. */
  ECOMM_RED_ACTION_IGNORE   = 2,
} ecomm_red_action_type_t;


// -----[ ecomm_red_filter_type_t ]----------------------------------
/** BGP Redistribution Communities: filter types.
 *  (draft-ietf-grow-bgp-redistribution-00). */
typedef enum {
  ECOMM_RED_FILTER_AS  = 0,
  ECOMM_RED_FILTER_2AS = 1,
  ECOMM_RED_FILTER_CIDR= 2,
  ECOMM_RED_FILTER_AS4 = 3,
} ecomm_red_filter_type_t;


#define ECOMM_DUMP_RAW 0
#define ECOMM_DUMP_TEXT 1

// -----[ bgp_ecomm_t ]----------------------------------------------
/** Definition of a single Extended Community value (RFC4360). */
typedef struct {
  unsigned char iana_authority:1;
  unsigned char transitive:1;
  unsigned char type_high:6;
  unsigned char type_low;
  unsigned char value[6];
} bgp_ecomm_t;


// -----[ bgp_ecomms_t ]---------------------------------------------
/** Definition of an Extended-Communities attribute (RFC4360). */
typedef struct {
  /** Number of values. */
  unsigned char num;
  /** Values. */
  bgp_ecomm_t   values[0];
} bgp_ecomms_t;


#endif /* __BGP_ATTR_TYPES_H__ */
