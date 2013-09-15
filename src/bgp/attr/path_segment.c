// ==================================================================
// @(#)path_segment.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 28/10/2003
// $Id: path_segment.c,v 1.1 2009-03-24 13:41:26 bqu Exp $
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
#include <string.h>

#include <libgds/stream.h>
#include <libgds/memory.h>
#include <libgds/tokenizer.h>
#include <bgp/attr/path.h>
#include <bgp/attr/path_segment.h>
#include <net/util.h>

// -----[ path segment delimiters ]-----
/**
 * These are the delimiters used in route_btoa.
 */
#define PATH_SEG_SET_DELIMS            "[]"
/**
 * These are the delimiters used in libbgpdump.
 */
//#define PATH_SEG_SET_DELIMS            "{}"
//#define PATH_SEG_CONFED_SET_DELIM      "[]"
//#define PATH_SEG_CONFED_SEQUENCE_DELIM "()"

static gds_tokenizer_t * segment_tokenizer= NULL;

// ----- path_segment_create ----------------------------------------
/**
 * Create an AS-Path segment of the given type and length.
 */
bgp_path_seg_t * path_segment_create(uint8_t type, uint8_t length)
{
  bgp_path_seg_t * seg=
    (bgp_path_seg_t *) MALLOC(sizeof(bgp_path_seg_t)+
			     (length * sizeof(asn_t)));
  seg->type= type;
  seg->length= length;
  return seg;
}

// ----- path_segment_destroy ---------------------------------------
/**
 * Destroy an AS-Path segment.
 */
void path_segment_destroy(bgp_path_seg_t ** seg_ref)
{
  if (*seg_ref != NULL) {
    FREE(*seg_ref);
    *seg_ref= NULL;
  }
}

// ----- path_segment_copy ------------------------------------------
/**
 * Duplicate an existing AS-Path segment.
 */
bgp_path_seg_t * path_segment_copy(const bgp_path_seg_t * seg)
{
  bgp_path_seg_t * new_seg=
    path_segment_create(seg->type, seg->length);
  memcpy(new_seg->asns, seg->asns, new_seg->length*sizeof(asn_t));
  return new_seg;
}

// -----[ path_segment_to_string ]-----------------------------------
/**
 * Convert the given AS-Path segment to a string. The string memory
 * MUST have been allocated before. The function will not write
 * outside of the allocated buffer, based on the provided destination
 * buffer size.
 *
 * Return value:
 *   -1    if the destination buffer is too small
 *   >= 0  otherwise (number of characters written without \0)
 *
 * Note: the function uses snprintf() in order to write into the
 * destination buffer. The return value of snprintf() is important. A
 * return value equal or larger that the maximum destination size
 *  means that the output was truncated.
 */
int path_segment_to_string(const bgp_path_seg_t * seg, int reverse,
			   char * dst, size_t dst_size)
{
  size_t tInitialSize= dst_size;
  unsigned int index;
  int iWritten= 0;

  assert(seg->length > 0);

  if (seg->type == 0) {
    fprintf(stderr, "\n*** plop %d ***\n", seg->length);
    fprintf(stderr, "type:%d content:", seg->type);
    for (index= 0; index < seg->length; index++) {
      fprintf(stderr, "%u ", seg->asns[index]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "\n*** plop ***\n");
  }

  assert((seg->type == AS_PATH_SEGMENT_SET) ||
	 (seg->type == AS_PATH_SEGMENT_SEQUENCE) ||
	 (seg->type == AS_PATH_SEGMENT_CONFED_SET) ||
	 (seg->type == AS_PATH_SEGMENT_CONFED_SEQUENCE));

  if (seg->type != AS_PATH_SEGMENT_SEQUENCE) {
    iWritten= snprintf(dst, dst_size, "}");
    if (iWritten == dst_size)
      return -1;
    dst_size-= iWritten;
    dst+= iWritten;
  }

  if (reverse) {
    for (index= seg->length; index > 0; index--) {
      if (index < seg->length)
	iWritten= snprintf(dst, dst_size, " %u", seg->asns[index-1]);
      else
	iWritten= snprintf(dst, dst_size, "%u", seg->asns[index-1]);	  
      if (iWritten >= dst_size)
	return -1;
      dst_size-= iWritten;
      dst+= iWritten;
    }
  } else {
    for (index= 0; index < seg->length; index++) {
      if (index > 0)
	iWritten= snprintf(dst, dst_size, " %u", seg->asns[index-1]);
      else
	iWritten= snprintf(dst, dst_size, "%u", seg->asns[index-1]);
      if (iWritten >= dst_size)
	return -1;
      dst_size-= iWritten;
      dst+= iWritten;
    }
  }

  if (seg->type != AS_PATH_SEGMENT_SEQUENCE) {
    iWritten= snprintf(dst, dst_size, "}");
    if (iWritten >= dst_size)
      return -1;
    dst_size-= iWritten;
    dst+= iWritten;
  }

  return tInitialSize-dst_size;
}

// -----[ path_segment_from_string ]---------------------------------
/**
 * Convert a string to an AS-Path segment.
 *
 * Return value:
 *   NULL  if the string does not contain a valid AS-Path
 *   a new AS-Path segment otherwise
 */
bgp_path_seg_t * path_segment_from_string(const char * seg_str)
{
  unsigned int index;
  const gds_tokens_t * tokens;
  const char * asn_str;
  bgp_path_seg_t * seg;
  asn_t asn;

  if (segment_tokenizer == NULL)
    segment_tokenizer= tokenizer_create(" ", NULL, NULL);

  if (tokenizer_run(segment_tokenizer, seg_str) != TOKENIZER_SUCCESS) {
    STREAM_ERR(STREAM_LEVEL_SEVERE, "Error: parse error in 'path_segment_from_string'\n");
    return NULL;
  }

  tokens= tokenizer_get_tokens(segment_tokenizer);
  seg= path_segment_create(AS_PATH_SEGMENT_SET, 0);
  for (index= tokens_get_num(tokens); index > 0; index--) {
    asn_str= tokens_get_string_at(tokens, index-1);
    if (str2asn(asn_str, &asn)) {
      STREAM_ERR(STREAM_LEVEL_SEVERE, "Error: invalid AS-Num \"%s\"\n",
		 asn_str);
      path_segment_destroy(&seg);
      break;
    }
    path_segment_add(&seg, asn);
  }
  return seg;
}

// ----- path_segment_dump ------------------------------------------
/**
 * Dump an AS-Path segment.
 */
void path_segment_dump(gds_stream_t * stream, const bgp_path_seg_t * seg,
		       int reverse)
{
  unsigned int index;

  switch (seg->type) {
  case AS_PATH_SEGMENT_SEQUENCE:
    if (reverse) {
      for (index= seg->length; index > 0; index--) {
	stream_printf(stream, "%u", seg->asns[index-1]);
	if (index > 1)
	  stream_printf(stream, " ");
      }
    } else {
      for (index= 0; index < seg->length; index++) {
	if (index > 0)
	  stream_printf(stream, " ");
	stream_printf(stream, "%u", seg->asns[index]);
      }
    }
    break;
  case AS_PATH_SEGMENT_SET:
    stream_printf(stream, "{");
    if (reverse) {
      for (index= seg->length; index > 0; index--) {
	stream_printf(stream, "%u", seg->asns[index-1]);
	if (index > 1)
	  stream_printf(stream, " ");
      }
    } else {
      for (index= 0; index < seg->length; index++) {
	if (index > 0)
	  stream_printf(stream, " ");
	stream_printf(stream, "%u", seg->asns[index]);
      }
    }
    stream_printf(stream, "}");
    break;
  default:
    abort();
  }
}

// -----[ _path_segment_resize ]-------------------------------------
/**
 * Resizes an existing segment.
 */
static inline void _path_segment_resize(bgp_path_seg_t ** seg_ref,
					uint8_t new_length)
{
  if (new_length == (*seg_ref)->length)
    return;

  *seg_ref= (bgp_path_seg_t *) REALLOC(*seg_ref,
				       sizeof(bgp_path_seg_t)+
				       new_length * sizeof(asn_t));
  (*seg_ref)->length= new_length;
}

// ----- path_segment_add -------------------------------------------
/**
 * Add a new AS number to the given segment. Since the size of the
 * segment will be changed by this function, the segment's location in
 * memory can change. Thus, the pointer to the segment can change !!!
 *
 * The segment's size is limited to 255 AS numbers. If the segment can
 * not hold a additional AS number, the function returns
 * -1. Otherwize, the new AS number is added and the function returns
 * 0.
 */
int path_segment_add(bgp_path_seg_t ** seg_ref, asn_t asn)
{
  if ((*seg_ref)->length >= MAX_PATH_SEQUENCE_LENGTH)
    return -1;

  _path_segment_resize(seg_ref, (*seg_ref)->length+1);
  (*seg_ref)->asns[(*seg_ref)->length-1]= asn;
  return 0;
}

// ----- path_segment_contains --------------------------------------
/**
 * Test if the given segment contains the given AS number.
 *
 * Return value:
 *   1  if the segment contains the ASN
 *   0  otherwise
 */
inline int path_segment_contains(const bgp_path_seg_t * seg, asn_t asn)
{
  unsigned int index;

  for (index= 0; index < seg->length; index++) {
    if (seg->asns[index] == asn)
      return 1;
  }
  return 0;
}

// ----- path_segment_equals ----------------------------------------
/**
 * Test if two segments are equal. If they are equal, the function
 * returns 1 otherwize, the function returns 0.
 */
inline int path_segment_equals(const bgp_path_seg_t * seg1,
			       const bgp_path_seg_t * seg2)
{
  unsigned int index;

  if (seg1 == seg2)
    return 1;

  if ((seg1->type != seg2->type) ||
      (seg1->length != seg2->length))
    return 0;
  
  switch (seg1->type) {
  case AS_PATH_SEGMENT_SEQUENCE:
    // In the case of an AS-SEQUENCE, we simply compare the memory
    // regions since the ordering of the AS numbers must be equal.
    if (memcmp(seg1->asns, seg2->asns,
	       seg1->length*sizeof(asn_t)) == 0)
      return 1;
    break;
  case AS_PATH_SEGMENT_SET:
    // In the case of an AS-SET, there is no ordering of the AS
    // numbers. We must thus check if each element of the first set is
    // in the second set. This is sufficient since we have already
    // compared the segment lengths.
    for (index= 0; index < seg1->length; index++)
      if (path_segment_contains(seg2, seg1->asns[index]) < -1)
	return 0;
    return 1;
    break;
  default:
    abort();
  }
  return 0;
}

// -----[ path_segment_cmp ]-----------------------------------------
/**
 * Compare two path segments. This function is aimed at sorting path
 * segments, not at measuring their length in the decision process !
 *
 * Pre-condition: path segments must not be NULL.
 *
 * Return value:
 *   -1  if seg1 < seg2
 *   0   if seg1 == seg2
 *   1   if seg1 > seg2
 *
 * Note: in the case of segments of type SEQUENCE, two segments with
 *       the same content might be considered different when their
 *       content is ordered differently.
 */
int path_segment_cmp(const bgp_path_seg_t * seg1,
		     const bgp_path_seg_t * seg2)
{
  unsigned int index;

  assert((seg1 != NULL) && (seg2 != NULL));

  // Longest is first
  if (seg1->length < seg2->length)
    return -1;
  else if (seg1->length > seg2->length)
    return 1;

  // Equal size segments, compare each AS number individually
  for (index= 0; index < seg1->length; index++) {
    if (seg1->asns[index] < seg2->asns[index])
      return -1;
    else if (seg1->asns[index] > seg2->asns[index])
      return 1;
  }

  return 0;
}

// -----[ path_segment_remove_private ]------------------------------
/**
 * Remove private ASNs from the segment. Private ASN are those in the
 * range [64512, 65535].
 */
bgp_path_seg_t * path_segment_remove_private(bgp_path_seg_t * seg)
{
  unsigned int index;
  int resize= 0;
  
  index= 0;
  while (index < seg->length) {

    // ASN in [64512, 65535] ?
    if ((seg->asns[index] >= 64512)) {
      memmove(&seg->asns[index],
	      &seg->asns[index+1],
	      sizeof(asn_t)*(seg->length-index-1));
      seg->length--;
      resize= 1;
    } else
      index++;

  }

  // ASNs were removed. Need to resize segment ?
  if (resize) {

    // No more ASN in segment => free
    if (seg->length == 0) {
      FREE(seg);
      return NULL;
    }

    // Resize
    return (bgp_path_seg_t *) REALLOC(seg,
				    sizeof(bgp_path_seg_t)+
				    seg->length*sizeof(asn_t));
  }

  return seg;
}

// -----[ _path_segment_destroy ]------------------------------------
void _path_segment_destroy()
{
  tokenizer_destroy(&segment_tokenizer);
}
