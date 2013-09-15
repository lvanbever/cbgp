// ==================================================================
// @(#)comm.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/05/2003
// $Id: comm.c,v 1.1 2009-03-24 13:41:26 bqu Exp $
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

#include <stdlib.h>
#include <string.h>

#include <libgds/memory.h>
#include <libgds/str_util.h>
#include <libgds/tokenizer.h>

#include <bgp/attr/comm.h>

/**
 * Note: as we case 'uint32_t' variables to 'unsigned int' for
 *       printing, we must make sure that 'unsigned int' is at least
 *       as large as 'uint32_t'
 */
#if UINT32_MAX > UINT_MAX
# error "unsigned int variables are too small"
#endif

static gds_tokenizer_t * pCommTokenizer= NULL;

// -----[ comms_create ]---------------------------------------------
bgp_comms_t * comms_create()
{
  bgp_comms_t * comms= (bgp_comms_t *) MALLOC(sizeof(bgp_comms_t));
  comms->num= 0;
  return comms;
}

// -----[ comms_destroy °--------------------------------------------
void comms_destroy(bgp_comms_t ** comms_ref)
{
  bgp_comms_t * comms= *comms_ref;
  if (comms == NULL)
    return;
  FREE(comms);
  *comms_ref= NULL;
}

// ----[ comms_dup ]-------------------------------------------------
bgp_comms_t * comms_dup(const bgp_comms_t * comms)
{
  bgp_comms_t * new_comms=
    (bgp_comms_t *) MALLOC(sizeof(bgp_comms_t)+
			     sizeof(bgp_comm_t)*comms->num);
  memcpy(new_comms, comms, sizeof(bgp_comms_t)+
	 sizeof(bgp_comm_t)*comms->num);
  return new_comms;
}

// -----[ comms_add ]------------------------------------------------
/**
 * Add a community value to the given Communities attribute.
 *
 * Return value:
 *    0   in case of success
 *   -1   in case of failure
 */
int comms_add(bgp_comms_t ** comms_ref, bgp_comm_t comm)
{
  assert((*comms_ref)->num < 255);
  (*comms_ref)->num++;
  (*comms_ref)=
    (bgp_comms_t *) REALLOC(*comms_ref, sizeof(bgp_comms_t)+
			    sizeof(bgp_comm_t)*((*comms_ref)->num));
    
  if (*comms_ref == NULL)
    return -1;
  (*comms_ref)->values[(*comms_ref)->num-1]= comm;
  return 0;
}

// -----[ comms_remove ]---------------------------------------------
/**
 * Remove a given community value from the set.
 */
void comms_remove(bgp_comms_t ** comms_ref, bgp_comm_t comm)
{
  unsigned int index;
  unsigned int last_index= 0;
  bgp_comms_t * comms= *comms_ref;

  if (comms == NULL)
    return;

  // Remove the given community value.
  for (index= 0; index < comms->num; index++) {
    if (comms->values[index] != comm) {
      if (index != last_index)
	memcpy(&comms->values[last_index],
	       &comms->values[index], sizeof(bgp_comm_t));
      last_index++;
    }
  }

  // destroy or resize the set of communities, if needed.
  if (comms->num == last_index)
    return;

  comms->num= last_index;
  if (comms->num == 0) {
    FREE(*comms_ref);
    *comms_ref= NULL;
  } else {
    *comms_ref=
      (bgp_comms_t *) REALLOC(*comms_ref,
			      sizeof(bgp_comms_t)+
			      comms->num*sizeof(bgp_comm_t));
  }
}

// -----[ comms_contains ]-------------------------------------------
/**
 * Check if a given community value belongs to the communities
 * attribute.
 *
 * Return value:
 *   1  the community was found and return value is the index.
 *   0  otherwise
 */
int comms_contains(bgp_comms_t * comms, bgp_comm_t comm)
{
  unsigned int index= 0;

  if (comms == NULL)
    return 0;

  while (index < comms->num) {
    if (comms->values[index] == comm)
      return 1;
    index++;
  }
  return 0;
}

// -----[ comms_equals ]---------------------------------------------
/**
 *
 */
int comms_equals(const bgp_comms_t * comms1, const bgp_comms_t * comms2)
{
  if (comms1 == comms2)
    return 1;
  if ((comms1 == NULL) || (comms2 == NULL))
    return 0;
  if (comms1->num != comms2->num)
    return 0;
  return (memcmp(comms1->values, comms2->values,
		 comms1->num*sizeof(bgp_comm_t)) == 0);
}

// -----[ comm_value_from_string ]-----------------------------------
int comm_value_from_string(const char * comm_str, bgp_comm_t * comm)
{
  unsigned long ulComm;
  char * comm_str2;

  if (!strcmp(comm_str, "no-export")) {
    *comm= COMM_NO_EXPORT;
  } else if (!strcmp(comm_str, "no-advertise")) {
    *comm= COMM_NO_ADVERTISE;
#ifdef __EXPERIMENTAL__
  } else if (!strcmp(comm_str, "depref")) {
    *comm= COMM_DEPREF;
#endif
  } else {

    if (strchr(comm_str, ':') == NULL) {

      // Single part (0-UINT32_MAX)
      if ((str_as_ulong(comm_str, &ulComm) < 0) ||
	  (ulComm > UINT32_MAX))
	return -1;

      *comm= ulComm;

    } else {
      
      // Two parts (0-65535):(0-65535)
      ulComm= strtoul(comm_str, &comm_str2, 0);
      if (*comm_str2 != ':')
	return -1;
      if (ulComm > 65535)
	return -1;
      *comm= ulComm << 16;
      ulComm= strtoul(comm_str2+1, &comm_str2, 0);
      if (*comm_str2 != '\0')
	return -1;
      if (ulComm > 65535)
	return -1;
      *comm+= ulComm;

    }
  }
  return 0;
}

// -----[ comm_value_dump ]------------------------------------------
/**
 *
 */
void comm_value_dump(gds_stream_t * stream, bgp_comm_t comm, int text)
{
  if (text == COMM_DUMP_TEXT) {
    switch (comm) {
    case COMM_NO_EXPORT:
      stream_printf(stream, "no-export");
      break;
    case COMM_NO_ADVERTISE:
      stream_printf(stream, "no-advertise");
      break;
    default:
      stream_printf(stream, "%u:%u", (comm >> 16), (comm & 65535));
    }
  } else {
    stream_printf(stream, "%u:%u", (comm >> 16), (comm & 65535));
  }
}

// ----- comm_dump --------------------------------------------------
/*
 *
 */
void comm_dump(gds_stream_t * stream, const bgp_comms_t * comms,
	       int text)
{
  unsigned int index;
  bgp_comm_t comm;

  for (index= 0; index < comms->num; index++) {
    if (index > 0)
      stream_printf(stream, " ");
    comm= comms->values[index];
    comm_value_dump(stream, comm, text);
  }
}

// -----[ comm_to_string ]-------------------------------------------
/**
 * Convert the given Communities to a string. The string memory MUST
 * have been allocated before. The function will not write outside of
 * the allocated buffer, based on the provided destination buffer
 * size.
 *
 * Return value:
 *   -1    not enough space in destination buffer
 *   >= 0  number of characters written
 */
int comm_to_string(bgp_comms_t * comms, char * pBuffer,
		   size_t tBufferSize)
{
  size_t tInitialSize= tBufferSize;
  int written= 0;
  unsigned int index;
  bgp_comm_t comm;

  if ((comms == NULL) || (comms->num == 0)) {
    if (tBufferSize < 1)
      return -1;
    pBuffer[0]= '\0';
    return 0;
  }
 
  for (index= 0; index < comms->num; index++) {
    comm= comms->values[index];

    if (index > 0) {
      written= snprintf(pBuffer, tBufferSize, " %u", (unsigned int) comm);
    } else {
      written= snprintf(pBuffer, tBufferSize, "%u", (unsigned int) comm);
    }
    if (written >= tBufferSize)
      return -1;
    tBufferSize-= written;
    pBuffer+= written;
  }
  return tInitialSize-tBufferSize;
}

// -----[ comm_from_string ]-----------------------------------------
/**
 *
 */
bgp_comms_t * comm_from_string(const char * comms_str)
{
  unsigned int index;
  bgp_comms_t * comms= NULL;
  const gds_tokens_t * tokens;
  bgp_comm_t comm;

  if (pCommTokenizer == NULL)
    pCommTokenizer= tokenizer_create(" ", NULL, NULL);

  if (tokenizer_run(pCommTokenizer, comms_str)
      != TOKENIZER_SUCCESS)
    return NULL;

  tokens= tokenizer_get_tokens(pCommTokenizer);

  comms= comms_create();
  for (index= 0; index < tokens_get_num(tokens); index++) {
    if (comm_value_from_string(tokens_get_string_at(tokens, index),
			       &comm)) {
      comms_destroy(&comms);
      return NULL;
    }
    comms_add(&comms, comm);
  }
  
  return comms;
}

// -----[ comms_cmp ]-------------------------------------------------
/**
 * Compare two Communities. This is just used for storing different
 * communities in a sorted array, not for testing that two Communities
 * are equal. Indeed, individual community values are not sorted in the
 * Communities which means that this function can say that two
 * Communities are different even if they contain the same set of
 * community values, but in different orderings.
 *
 * Return value:
 *   0  communities are equal
 *   <0 comm1 < comm2
 *   >0 comm1 > comm2
 */
int comms_cmp(const bgp_comms_t * comm1, const bgp_comms_t * comm2)
{
  unsigned int index;

  // Pointers are equal means Communities are equal
  if (comm1 == comm2)
    return 0;

  // Empty and null Communities are equal
  if (((comm1 == NULL) && (comm2->num == 0)) ||
      ((comm2 == NULL) && (comm1->num == 0)))
    return 0;

  if (comm1 == NULL)
    return -1;
  if (comm2 == NULL)
    return 1;

  // Check Communities length
  if (comm1->num < comm2->num)
    return -1;
  else if (comm1->num > comm2->num)
    return 1;

  // Check individual community values
  for (index= 0; index < comm1->num; index++) {
    if (comm1->values[index] < comm2->values[index])
      return -1;
    else if (comm1->values[index] > comm2->values[index])
      return 1;
  }

  // Both Communities are equal
  return 0;
}

// -----[ _comm_destroy ]--------------------------------------------
void _comm_destroy()
{
  tokenizer_destroy(&pCommTokenizer);
}
