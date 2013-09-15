// ==================================================================
// @(#)path.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/12/2002
// $Id: path.c,v 1.1 2009-03-24 13:41:26 bqu Exp $
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
#include <bgp/attr/path_hash.h>
#include <bgp/attr/path_segment.h>
#include <bgp/filter/filter.h>

static gds_tokenizer_t * path_tokenizer= NULL;

// ----- _path_destroy_segment --------------------------------------
static void _path_destroy_segment(void * item, const void * ctx)
{
  path_segment_destroy((bgp_path_seg_t **) item);
}

// ----- path_create ------------------------------------------------
/**
 * Creates an empty AS-Path.
 */
bgp_path_t * path_create()
{
  bgp_path_t * path;
#ifndef  __BGP_PATH_TYPE_TREE__
  path= (bgp_path_t *) ptr_array_create(0, NULL,
					_path_destroy_segment, NULL);
#else
  path= (bgp_path_t *) MALLOC(sizeof(bgp_path_t));
  path->segment= NULL;
  path->pPrevious= NULL;
#endif
  return path;
}

// ----- path_destroy -----------------------------------------------
/**
 * Destroy an existing AS-Path.
 */
void path_destroy(bgp_path_t ** ppath)
{
#ifndef __BGP_PATH_TYPE_TREE__
  ptr_array_destroy(ppath);
#else
  if (*ppath != NULL) {
    if ((*ppath)->pPrevious != NULL)
      path_hash_remove((*ppath)->pPrevious);
    if ((*ppath)->segment != NULL)
      path_segment_destroy(&(*ppath)->segment);
    *ppath= NULL;
  }
#endif
}

// ----- path_max_value ---------------------------------------------
/**
  * Create a path and sets the first ASN value to the maximum
  */
bgp_path_t * path_max_value()
{
  bgp_path_t * path = path_create();

  path_append(&path, 65535U);
  
  return path;
} 

// ----- path_copy --------------------------------------------------
/**
 * Duplicate an existing AS-Path.
 */
bgp_path_t * path_copy(bgp_path_t * path)
{
  bgp_path_t * new_path= NULL;
#ifndef __BGP_PATH_TYPE_TREE__
  unsigned int index;
#endif

  if (path == NULL)
    return NULL;

#ifndef __BGP_PATH_TYPE_TREE__
  new_path= path_create();
  for (index= 0; index < path_num_segments(path); index++)
    path_add_segment(new_path, path_segment_copy((bgp_path_seg_t *) 
						 path->data[index]));
#else
  cbgp_fatal("not implemented");
#endif
  return new_path;
}

// ----- path_num_segments ------------------------------------------
/**
 * Return the number of segments in the given AS-Path.
 */
int path_num_segments(const bgp_path_t * path)
{
  if (path != NULL) {
#ifndef __BGP_PATH_TYPE_TREE__
    return ptr_array_length((ptr_array_t *) path);
#else
    return 1;
#endif
  }
  return 0;
}

// -----[ _path_segment_at ]-----------------------------------------
#define _path_segment_at(P, I) (bgp_path_seg_t *) (P)->data[(I)]
#define _path_segment_ref_at(P, I) (bgp_path_seg_t **) &((P)->data[(I)])

// ----- path_length ------------------------------------------------
/**
 * Return the number of AS hops in the given AS-Path. The length of an
 * AS-SEQUENCE segment is equal to the number of ASs which compose
 * it. The length of an AS-SET is equal to 1.
 */
int path_length(bgp_path_t * path)
{
  int length= 0;
#ifndef __BGP_PATH_TYPE_TREE__
  unsigned int index;
  bgp_path_seg_t * seg;

  if (path == NULL)
    return 0;

  for (index= 0; index < path_num_segments(path); index++) {
    seg= _path_segment_at(path, index);
    switch (seg->type) {
    case AS_PATH_SEGMENT_SEQUENCE:
      length+= seg->length;
      break;
    case AS_PATH_SEGMENT_SET:
      length++;
      break;
    default:
      abort();
    }
  }
#else
  cbgp_fatal("not implemented");
#endif
  return length;
}

// ----- path_add_segment -------------------------------------------
/**
 * Add the given segment to the given path.
 *
 * Return value:
 *   >= 0   in case of success
 *   -1     in case of failure
 */
int path_add_segment(bgp_path_t * path, bgp_path_seg_t * seg)
{
#ifndef __BGP_PATH_TYPE_TREE__
  return ptr_array_append((ptr_array_t *) path, seg);
#else
  cbgp_fatal("not implemented");
#endif
}

// ----- path_append ------------------------------------------------
/**
 * Append a new AS to the given AS-Path. If the last segment of the
 * AS-Path is an AS-SEQUENCE, the new AS is simply added at the end of
 * this segment. If the last segment is an AS-SET, a new segment of
 * type AS-SEQUENCE which contains only the new AS is added at the end
 * of the AS-Path.
 *
 * Return value:
 *   length(new AS-Path)   in case of success
 *   -1                    in case of error
 */
int path_append(bgp_path_t ** ppath, asn_t asn)
{
  int length= path_num_segments(*ppath);

#ifndef __BGP_PATH_TYPE_TREE__
  bgp_path_seg_t * seg;

  if (length <= 0) {
    seg= path_segment_create(AS_PATH_SEGMENT_SEQUENCE, 1);
    seg->asns[0]= asn;
    path_add_segment(*ppath, seg);
  } else {

    // Find last segment.
    // - if it is a SEQUENCE, append.
    // - if it is a SET, create new segment.
    seg= _path_segment_at(*ppath, length-1);
    switch (seg->type) {
    case AS_PATH_SEGMENT_SEQUENCE:
      if (path_segment_add(_path_segment_ref_at(*ppath, length-1), asn))
	return -1;
      break;
    case AS_PATH_SEGMENT_SET:
      seg= path_segment_create(AS_PATH_SEGMENT_SEQUENCE, 1);
      seg->asns[0]= asn;
      path_add_segment(*ppath, seg);
      length++;
      break;
    default:
      abort();
    }
  }
#else
  cbgp_fatal("not implemented");
#endif
  return length;
}

// ----- path_contains ----------------------------------------------
/**
 * Test if the given AS-Path contains the given AS number.
 *
 * Return value:
 *   1  if the path contains the ASN
 *   0  otherwise
 */
int path_contains(bgp_path_t * path, asn_t asn)
{
#ifndef __BGP_PATH_TYPE_TREE__
  unsigned int index;

  if (path == NULL)
    return 0;

  for (index= 0; index < path_num_segments(path); index++) {
    if (path_segment_contains((bgp_path_seg_t *) path->data[index], asn) != 0)
      return 1;
  }
#else
  cbgp_fatal("not implemented");
#endif
  return 0;
}

// ----- path_at ----------------------------------------------------
/**
 * Return the AS number at the given position whatever the including
 * segment is.
 */
int path_at(bgp_path_t * path, int pos)
{
#ifndef __BGP_PATH_TYPE_TREE__
  unsigned int index, seg_index;
  bgp_path_seg_t * seg;

  for (index= 0; (pos > 0) && 
	 (index < path_num_segments(path));
       index++) {
    seg= _path_segment_at(path, index);
    switch (seg->type) {
    case AS_PATH_SEGMENT_SEQUENCE:
      for (seg_index= 0; seg_index ; seg_index++) {
	if (pos == 0)
	  return seg->asns[seg_index];
	pos++;
      }
      break;
    case AS_PATH_SEGMENT_SET:
      if (pos == 0)
	return seg->asns[0];
      pos++;
      break;
    default:
      abort();
    }
  }
#else
  cbgp_fatal("not implemented");
#endif
  return -1;
}

// -----[ path_last_as ]---------------------------------------------
/**
 * Return the last AS-number in the AS-Path.
 *
 * Note: this function does not work if the last segment in the
 * AS-Path is of type AS-SET since there is no ordering of the
 * AS-numbers in this case.
 */
int path_last_as(bgp_path_t * path) {
#ifndef __BGP_PATH_TYPE_TREE__
  bgp_path_seg_t * seg;

  if (path_length(path) <= 0)
    return -1;

  seg= (bgp_path_seg_t *)
    path->data[ptr_array_length(path)-1];
  
  // Check that the segment is of type AS_SEQUENCE 
  assert(seg->type == AS_PATH_SEGMENT_SEQUENCE);
  // Check that segment's size is larger than 0
  assert(seg->length > 0);

  return seg->asns[seg->length-1];
#else
  cbgp_fatal("not implemented");
#endif
}

// -----[ path_first_as ]--------------------------------------------
/**
 * Return the first AS-number in the AS-Path. This function can be
 * used to retrieve the origin AS.
 *
 * Note: this function does not work if the first segment in the
 * AS-Path is of type AS-SET since there is no ordering of the
 * AS-numbers in this case.
 */
int path_first_as(bgp_path_t * path) {
#ifndef __BGP_PATH_TYPE_TREE__
  bgp_path_seg_t * seg;

  if (path_length(path) <= 0)
    return -1;

  seg= (bgp_path_seg_t *) path->data[0];

  // Check that the segment is of type AS_SEQUENCE 
  assert(seg->type == AS_PATH_SEGMENT_SEQUENCE);
  // Check that segment's size is larger than 0
  assert(seg->length > 0);

  return seg->asns[0];
#else
  cbgp_fatal("not implemented");
#endif
}

// -----[ path_to_string ]-------------------------------------------
/**
 * Convert the given AS-Path to a string. The string memory MUST have
 * been allocated before. The function will not write outside of the
 * allocated buffer, based on the provided destination buffer size.
 *
 * Return value:
 *   -1    if the destination buffer is too small
 *   >= 0  number of characters written (without \0)
 *
 * Note: the function uses snprintf() in order to write into the
 * destination buffer. The return value of snprintf() is important. A
 * return value equal or larger that the maximum destination size
 * means that the output was truncated.
 */
int path_to_string(bgp_path_t * path, int reverse,
		   char * dst, size_t dst_size)
{
#ifndef __BGP_PATH_TYPE_TREE__
  size_t tInitialSize= dst_size;
  unsigned int index;
  int iWritten= 0;
  unsigned int num_segs;

  if ((path == NULL) || (path_num_segments(path) == 0)) {
    if (dst_size < 1)
      return -1;
    *dst= '\0';
    return 0;
  }

  if (reverse) {
    num_segs= path_num_segments(path);
    for (index= num_segs; index > 0; index--) {
      // Write space (if required)
      if (index < num_segs) {
	iWritten= snprintf(dst, dst_size, " ");
	if (iWritten >= dst_size)
	  return -1;
	dst+= iWritten;
	dst_size-= iWritten;
      }

      // Write AS-Path segment
      iWritten= path_segment_to_string((bgp_path_seg_t *) path->data[index-1],
				       reverse, dst, dst_size);
      if (iWritten >= dst_size)
	return -1;
      dst+= iWritten;
      dst_size-= iWritten;
    }

  } else {
    for (index= 0; index < path_num_segments(path);
	 index++) {
      // Write space (if required)
      if (index > 0) {
	iWritten= snprintf(dst, dst_size, " ");
	if (iWritten >= dst_size)
	  return -1;
	dst+= iWritten;
	dst_size-= iWritten;
      }
      // Write AS-Path segment
      iWritten= path_segment_to_string((bgp_path_seg_t *) path->data[index],
				       reverse, dst, dst_size);
      if (iWritten >= dst_size)
	return -1;
      dst+= iWritten;
      dst_size-= iWritten;
    }
  }
  return tInitialSize-dst_size;
#else
  cbgp_fatal("not implemented");
#endif
}

// -----[ path_from_string ]-----------------------------------------
/**
 * Convert a string to an AS-Path.
 *
 * Return value:
 *   NULL  if the string does not contain a valid AS-Path
 *   new path otherwise
 */
bgp_path_t * path_from_string(const char * path_str)
{
  unsigned int index;
  const gds_tokens_t * tokens;
  bgp_path_t * path= NULL;
  const char * seg_str;
  bgp_path_seg_t * seg;
  unsigned long asn;

  if (path_tokenizer == NULL)
    path_tokenizer= tokenizer_create(" ", "{[(", "}])");

  if (tokenizer_run(path_tokenizer, path_str) != TOKENIZER_SUCCESS)
    return NULL;

  tokens= tokenizer_get_tokens(path_tokenizer);

  path= path_create();
  for (index= tokens_get_num(tokens); index > 0; index--) {
    seg_str= tokens_get_string_at(tokens, index-1);

    if (!tokens_get_ulong_at(tokens, index-1, &asn)) {
      if (asn > MAX_AS) {
	STREAM_ERR(STREAM_LEVEL_SEVERE, "Error: not a valid AS-Num \"%s\"\n",
		   seg_str);
	path_destroy(&path);
	break;
      }
      path_append(&path, asn);
    } else {
      seg= path_segment_from_string(seg_str);
      if (seg == NULL) {
	STREAM_ERR(STREAM_LEVEL_SEVERE,
		   "Error: not a valid path segment \"%s\"\n", seg_str);
	path_destroy(&path);
	break;
      }
      path_add_segment(path, seg);
    }
  }

  return path;
}

// ----- path_match --------------------------------------------------------
/**
 *
 */
int path_match(bgp_path_t * path, SRegEx * pRegEx)
{
  char acBuffer[1000];
  int iRet = 0;

  if (path_to_string(path, 1, acBuffer, sizeof(acBuffer)) < 0)
    abort();

  if (pRegEx != NULL) {
    if (strcmp(acBuffer, "null") != 0) {
      if (regex_search(pRegEx, acBuffer) > 0)
        iRet = 1;
      regex_reinit(pRegEx);
    }
  }

  return iRet;
}

// ----- path_dump --------------------------------------------------
/**
 * Dump the given AS-Path to the given ouput stream.
 */
void path_dump(gds_stream_t * stream, const bgp_path_t * path,
	       int reverse)
{
#ifndef __BGP_PATH_TYPE_TREE__
  int index;

  if ((path == NULL) || (path_num_segments(path) == 0)) {
    stream_printf(stream, "");
    return;
  }

  if (reverse) {
    for (index= path_num_segments(path); index > 0; index--) {
      path_segment_dump(stream, (bgp_path_seg_t *) path->data[index-1],
			reverse);
      if (index > 1)
	stream_printf(stream, " ");
    }
  } else {
    for (index= 0; index < path_num_segments(path); index++) {
      if (index > 0)
	stream_printf(stream, " ");
      path_segment_dump(stream, (bgp_path_seg_t *) path->data[index],
			reverse);
    }
  }
#else
  cbgp_fatal("not implemented");
#endif
}

// ----- path_hash --------------------------------------------------
/**
 * Universal hash function for string keys (discussed in Sedgewick's
 * "Algorithms in C, 3rd edition") and adapted for AS-Paths.
 */
uint32_t path_hash(const bgp_path_t * path)
{
#ifndef __BGP_PATH_TYPE_TREE__
  uint32_t hash, a = 31415, b = 27183, M = 65521;
  unsigned int index, seg_index;
  bgp_path_seg_t * seg;

  if (path == NULL)
    return 0;

  hash= 0;
  for (index= 0; index < path_num_segments(path); index++) {
    seg= (bgp_path_seg_t *) path->data[index];
    for (seg_index= 0; seg_index < seg->length; seg_index++) {
      hash= (a * hash+seg->asns[seg_index]) % M;
      a= a * b % (M-1);
    }
  }
  return hash;
#else
  abort();
#endif
}

// -----[ path_hash_zebra ]------------------------------------------
/**
 * This is a helper function that computes the hash key of an
 * AS-Path. The function is based on Zebra's AS-Path hashing
 * function.
 */
uint32_t path_hash_zebra(const void * item, unsigned int hash_size)
{
#ifndef __BGP_PATH_TYPE_TREE__
  bgp_path_t * path= (bgp_path_t *) item;
  unsigned int hash= 0;
  uint32_t index, index2;
  bgp_path_seg_t * seg;

  for (index= 0; index < path_num_segments(path); index++) {
    seg= (bgp_path_seg_t *) path->data[index];
    // Note: segment type IDs are equal to those of Zebra
    //(1 AS_SET, 2 AS_SEQUENCE)
    hash+= seg->type;
    for (index2= 0; index2 < seg->length; index2++) {
      hash+= seg->asns[index2] & 255;
      hash+= seg->asns[index2] >> 8;
    }
  }
  return hash;
#else
  abort();
#endif
}

// -----[ path_hash_OAT ]--------------------------------------------
/**
 * Note: 'hash_size' must be a power of 2.
 */
uint32_t path_hash_OAT(const void * item, unsigned int hash_size)
{
#ifndef __BGP_PATH_TYPE_TREE__
  uint32_t hash= 0;
  bgp_path_t * path= (bgp_path_t *) item;
  uint32_t index, index2;
  bgp_path_seg_t * seg;

  for (index= 0; index < path_num_segments(path); index++) {
    seg= (bgp_path_seg_t *) path->data[index];
    // Note: segment type IDs are equal to those of Zebra
    //(1 AS_SET, 2 AS_SEQUENCE)

    hash+= seg->type;
    hash+= (hash << 10);
    hash^= (hash >> 6);
 
    for (index2= 0; index2 < seg->length; index2++) {
      hash+= seg->asns[index2] & 255;
      hash+= (hash << 10);
      hash^= (hash >> 6);
      hash+= seg->asns[index2] >> 8;
      hash+= (hash << 10);
      hash^= (hash >> 6);
    }
  }
  hash+= (hash << 3);
  hash^= (hash >> 11);
  hash+= (hash << 15);
  return hash % hash_size;
#else
  abort();
#endif
}

// ----- path_comparison --------------------------------------------
/**
  * Do a comparison ASN value by ASN value between two paths.
  * It does *not* take into account the length of the as-path first.
  * if you want to do so, then first use path_equals.
  * But if you have two paths as the following:
  * 1) 22 23 24 25
  * 2) 22 23 24
  *The first one will be considered as greater because of his length but ...
  * 1) 25 24 23 22
  * 2) 24 23 22
  * in this case the first will be considered as greater because of the first ASN value.
  */
int path_comparison(bgp_path_t * path1, bgp_path_t * path2)
{
  int length;
  unsigned int index;

  if (path_length(path1) < path_length(path2))
    length= path_length(path1);
  else
    length= path_length(path2);

  for (index = 0; index < length; index++) {
    if (path_at(path1, index) < path_at(path2, index))
      return -1;
    else
      if (path_at(path1, index) > path_at(path2, index))
        return 1;
  }
  return path_equals(path1, path2);
}

// ----- path_equals ------------------------------------------------
/**
 * Test if two paths are equal. If they are equal, the function
 * returns 1, otherwize, the function returns 0.
 */
int path_equals(const bgp_path_t * path1, const bgp_path_t * path2)
{
#ifndef __BGP_PATH_TYPE_TREE__
  unsigned int index;

  if (path1 == path2)
    return 1;

  if (path_num_segments(path1) != path_num_segments(path2))
    return 0;

  for (index= 0; index < path_num_segments(path1); index++) {
    if (!path_segment_equals((bgp_path_seg_t *) path1->data[index],
			     (bgp_path_seg_t *) path2->data[index]))
      return 0;
  }
  return 1;
#else
  abort();
#endif
}

// -----[ path_cmp ]-------------------------------------------------
/**
 * Compare two AS-Paths. This function is aimed at sorting AS-Paths,
 * not at measuring their length for the decision process !
 *
 * Return value:
 *   -1  if path1 < path2
 *   0   if path1 == path2
 *   1   if path1 > path2
 */
int path_cmp(const bgp_path_t * path1, const bgp_path_t * path2)
{
  unsigned int index;
  int iCmp;

  // Null paths are equal
  if (path1 == path2)
    return 0;

  // Null and empty paths are equal
  if (((path1 == NULL) && (path_num_segments(path2) == 0)) ||
      ((path2 == NULL) && (path_num_segments(path1) == 0)))
    return 0;

  if (path1 == NULL)
    return -1;
  if (path2 == NULL)
    return 1;
  
  // Longest is first
  if (path_num_segments(path1) < path_num_segments(path2))
    return -1;
  else if (path_num_segments(path1) > path_num_segments(path2))
    return 1;

  // Equal size paths, inspect individual segments
  for (index= 0; index < path_num_segments(path1); index++) {
    iCmp= path_segment_cmp((bgp_path_seg_t *) path1->data[index],
			   (bgp_path_seg_t *) path2->data[index]);
    if (iCmp != 0)
      return iCmp;
  }

  return 0;
}

// -----[ path_str_cmp ]---------------------------------------------
int path_str_cmp(bgp_path_t * path1, bgp_path_t * path2)
{
#define AS_PATH_STR_SIZE 1024
  char acPathStr1[AS_PATH_STR_SIZE];
  char acPathStr2[AS_PATH_STR_SIZE];

  // If paths pointers are equal, no need to compare their content.
  if (path1 == path2)
    return 0;

  assert(path_to_string(path1, 1, acPathStr1, AS_PATH_STR_SIZE) >= 0);
  assert(path_to_string(path2, 1, acPathStr2, AS_PATH_STR_SIZE) >= 0);
   
  return strcmp(acPathStr1, acPathStr2);
}

// -----[ path_remove_private ]--------------------------------------
void path_remove_private(bgp_path_t * path)
{
  unsigned int index;
  bgp_path_seg_t * seg;
  bgp_path_seg_t * new_seg;

  index= 0;
  while (index < path_num_segments(path)) {
    seg= (bgp_path_seg_t *) path->data[index];
    new_seg= path_segment_remove_private(seg);
    if (seg != new_seg) {
      if (new_seg == NULL) {
	ptr_array_remove_at(path, index);
	continue;
      } else {
	path->data[index]= new_seg;
      }
    }
    index++;
  }
}

// -----[ _path_destroy ]--------------------------------------------
void _path_destroy()
{
  tokenizer_destroy(&path_tokenizer);
}




