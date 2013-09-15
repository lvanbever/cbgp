// ==================================================================
// @(#)ntf.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/03/2005
// $Id: ntf.h,v 1.4 2009-03-24 16:08:30 bqu Exp $
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
 * Provide data structures and functions to load an NTF topology
 * file. Unfortunately, the standard Network Topology File (NTF) file
 * specification does not exists. So these functions are for "my" NTF
 * file format.
 *
 * My NTF file format is as follows. Each line is composed of 3 to 4
 * fields (i.e. latest is optionnal) and specifies a single
 * bidirectionnal link. Fields are delimited by single spaces or
 * tabulations.
 *
 *   <id1> <id2> <igp-weight> [ <delay> ]
 *
 * where
 * \li <id1> and <id2> are identifiers for the link's end nodes.
 *       These identifiers are IPv4 addresses specified in the dot
 *       format "a.b.c.d".
 * \li <igp-weight> is the link's IGP weight. The IGP weight is
 *       specified as an unsigned integer, i.e. in the range
 *       [0, 2^32[.
 * \li <delay> is an optionnal link delay. If not specified, the link's
 *       delay will be 0. The link delay is specified as an
 *       unsigned integer, i.e. in the range [0, 2^32[.
 *
 * \todo
 * All nodes and links are currently created on the fly. If there is
 * a problem in the NTF file (a duplicate link for example), all the
 * nodes and links loaded before the error will be present in the
 * topology. In case of NTF load error, there is no other choice than
 * stopping the simulator and starting it again. In the future, we
 * should load the content of the NTF file in a temporary data
 * structure, check it against all possible consistency errors, check
 * for conflicts with the current network database. It is only when
 * all checks have passed that changes are made to network topology.
 */

#ifndef __NTF_H__
#define __NTF_H__

// -----[ ntf_error_t ]----------------------------------------------
/** Definition of possible NTF error codes. */
typedef enum {
  /** Topology loaded succesfully. */
  NTF_SUCCESS              = 0,
  /** Syntax error. */
  NTF_ERROR_SYNTAX         = -1,
  /** Could not open file. */
  NTF_ERROR_OPEN           = -2,
  /** Invalid number of parameters (not in [3, 4]). */
  NTF_ERROR_NUM_PARAMS     = -3,
  /** Invalid IP address. */
  NTF_ERROR_INVALID_ADDRESS= -4,
  /** Invalid IGP weight. */
  NTF_ERROR_INVALID_WEIGHT = -5,
  /** Invalid delay. */
  NTF_ERROR_INVALID_DELAY  = -6,
} ntf_error_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ ntf_load ]-----------------------------------------------
  /**
   * Load an NTF file and create the corresponding nodes and links.
   *
   * \param filename is the name of the NTF file to be loaded.
   * \retval an error code.
   */
  int ntf_load(const char * filename);

  // -----[ ntf_save ]-----------------------------------------------
  /**
   * Save the current network topology to an NTF file.
   *
   * \param filename is the name of the output file.
   * \retval an error code.
   */
  int ntf_save(const char * filename);

  // -----[ ntf_strerror ]-------------------------------------------
  const char * ntf_strerror();
  // -----[ ntf_strerrorloc ]----------------------------------------
  const char * ntf_strerrorloc();

  // -----[ _ntf_init ]----------------------------------------------
  /**
   * \internal
   * This function must be called by the library initialization
   * function.
   */
  void _ntf_init();

  // -----[ _ntf_done ]----------------------------------------------
  /**
   * \internal
   * This function must be called by the library finalization
   * function.
   */
  void _ntf_done();

#ifdef __cplusplus
}
#endif

#endif /* __NTF_H__ */
