// ==================================================================
// @(#)cisco.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/05/2007
// $Id: cisco.c,v 1.3 2009-03-24 14:11:11 bqu Exp $
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
#include <config.h>
#endif

#include <libgds/tokenizer.h>
#include <libgds/tokens.h>

#include <bgp/cisco.h>

// ----- Example input file -----------------------------------------
// BGP table version is 253386, local router ID is 198.32.162.100
// Status codes: s suppressed, d damped, h history, * valid, > best, i - internal
// Origin codes: i - IGP, e - EGP, ? - incomplete
// 
//    Network          Next Hop          Metric LocPrf Weight Path
// *  3.0.0.0          206.157.77.11        165             0 1673 701 80 ?
// *                   144.228.240.93         4             0 1239 701 80 ?
// *                   205.238.48.3                         0 2914 1280 701 80 ?
// ------------------------------------------------------------------

// ----- Parser states -----
#define CISCO_STATE_HEADER  0
#define CISCO_STATE_RECORDS 1

// -----[ cisco_perror ]---------------------------------------------
void cisco_perror(gds_stream_t * stream, int error)
{
#define LOG(M) stream_printf(stream, M); break;
  switch (error) {
  case CISCO_SUCCESS:
    LOG("success");
  case CISCO_ERROR_UNEXPECTED:
    LOG("unexpected error");
  case CISCO_ERROR_FILE_OPEN:
    LOG("unable to open file");
  case CISCO_ERROR_NUM_FIELDS:
    LOG("incorrect number of fields");
  default:
    stream_printf(stream, "unknown error (%i)", error);
  }
#undef LOG
}

// -----[ cisco_record_parser ]--------------------------------------
int cisco_record_parser(const char * line)
{
  // First field
  switch (line[0]) {
  case '*':
    break;
  case ' ':
    break;
  default:
    return CISCO_ERROR_UNEXPECTED;
  }

  // Second field
  switch (line[1]) {
  case '>':
    break;
  case ' ':
    break;
  default:
    return CISCO_ERROR_UNEXPECTED;
  }

  // Space separator
  if (line[2] != ' ')
    return CISCO_ERROR_UNEXPECTED;

  // Get "Network" field (length=19)

  // Get "Next Hop" field (length=15)
  
  // Get "Metric" field (length=?)

  // Get "LocPrf" field (length=?)

  // Get "Weight" field (length=?)

  // Get "Path" field (length=variable)

  // Get "Origin" field

  return CISCO_SUCCESS;
}

// -----[ cisco_parser ]---------------------------------------------
int cisco_parser(FILE * stream)
{
  gds_tokenizer_t * tokenizer;
  const gds_tokens_t * tokens;
  char line[80];
  unsigned int line_number= 0;
  int error= CISCO_SUCCESS;

  tokenizer= tokenizer_create(" \t", NULL, NULL);

  while ((!feof(stream)) && (!error)) {
    if (fgets(line, sizeof(line), stream) == NULL)
      break;
    line_number++;

    // Skip comments starting with '#'
    if (line[0] == '#')
      continue;

    
    tokens= tokenizer_get_tokens(tokenizer);

  }
  tokenizer_destroy(&tokenizer);

  return error;
}

// -----[ cisco_load ]-----------------------------------------------
int cisco_load(const char * filename)
{
  FILE * pFile;
  int result;

  pFile= fopen(filename, "r");
  if (pFile == NULL)
    return -1;

  result= cisco_parser(pFile);

  fclose(pFile);
  return result;
}
