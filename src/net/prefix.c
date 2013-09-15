// ==================================================================
// @(#)prefix.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/11/2002
// $Id: prefix.c,v 1.23 2009-03-24 16:22:30 bqu Exp $
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

#include <assert.h>
#include <stdlib.h>
#include <libgds/stream.h>
#include <libgds/memory.h>
#include <net/prefix.h>

#include <string.h>

// -----[ ip_address_to_string ]-------------------------------------
/**
 * Convert an IPv4 address to a string.
 *
 * Return value:
 *   -1    if buffer too small
 *   >= 0  number of characters written
 */
int ip_address_to_string(net_addr_t addr, char * pcAddr, size_t tDstSize)
{
  int result= snprintf(pcAddr, tDstSize, "%u.%u.%u.%u",
			(unsigned int) (addr >> 24),
			(unsigned int) (addr >> 16) & 255, 
			(unsigned int) (addr >> 8) & 255,
			(unsigned int) addr & 255);
  return ((result < 0) || (result >= tDstSize)) ? -1 : result;
}

// ----- ip_address_dump --------------------------------------------
/**
 *
 */
void ip_address_dump(gds_stream_t * stream, net_addr_t addr)
{
  stream_printf(stream, "%u.%u.%u.%u",
	     (addr >> 24), (addr >> 16) & 255,
	     (addr >> 8) & 255, addr & 255);
}

// ----- ip_string_to_address ---------------------------------------
/**
 *
 */
int ip_string_to_address(const char * str, char ** end_ptr,
			 net_addr_t * addr)
{
  unsigned long int digit;
  uint8_t num_digits= 4;

  if (addr == NULL)
    return -1;
  *addr= 0;
  while (num_digits-- > 0) {
    digit= strtoul(str, end_ptr, 10);
    if ((str == *end_ptr) || (digit > 255) ||
	((num_digits > 0) && (**end_ptr != '.')))
      return -1;
    str= *end_ptr+1;
    *addr= (*addr << 8)+digit;
  }
  return 0;
}

// -----[ ip_address_cmp ]-------------------------------------------
/**
 * Compare 2 IPv4 addresses.
 *
 * Return:
 *   1 if A1 > A2
 *   0 if A1 == A2
 *  -1 if A1 < A2
 */
static inline
int _ip_address_cmp(net_addr_t addr1, net_addr_t addr2)
{
  if (addr1 > addr2)
    return 1;
  if (addr1 < addr2)
    return -1;
  return 0;
}

// -----[ _ip_address_mask ]------------------------------------------
/**
 * Create a network mask based on a mask length.
 *
 * Pre: mask length in range [0,32]
 */
static inline
net_addr_t _ip_address_mask(net_mask_t mask)
{
  assert(mask <= 32);
  // Warning, shift on 32-bit int is only defined if operand in [0-31]
  // Thus, special case for mask-length value of 0
  if (mask == 0)
    return 0;
  return (0xffffffff << (32-mask));
}

// ----- uint32_to_prefix -------------------------------------------
/**
 *
 */
ip_pfx_t uint32_to_prefix(net_addr_t addr, net_mask_t mask)
{
  ip_pfx_t prefix;
  prefix.network= addr;
  prefix.mask= mask;
  return prefix;
}


// ----- create_ip_prefix -------------------------------------------
ip_pfx_t * create_ip_prefix(net_addr_t addr, net_mask_t mask)
{
  ip_pfx_t * prefix = (ip_pfx_t *) MALLOC(sizeof(ip_pfx_t));
  prefix->network= addr;
  prefix->mask= mask;
  return prefix;
}

// ----- ip_prefix_dump ---------------------------------------------
/**
 *
 */
void ip_prefix_dump(gds_stream_t * stream, ip_pfx_t prefix)
{
  stream_printf(stream, "%u.%u.%u.%u/%u",
	     prefix.network >> 24, (prefix.network >> 16) & 255,
	     (prefix.network >> 8) & 255, (prefix.network & 255),
	     prefix.mask);
}

// ----- ip_prefix_to_string ----------------------------------------
/**
 *
 *
 */
int ip_prefix_to_string(const ip_pfx_t * prefix, char * str, size_t str_size)
{
  int result= snprintf(str, str_size, "%u.%u.%u.%u/%u", 
			(unsigned int) prefix->network >> 24,
			(unsigned int) (prefix->network >> 16) & 255,
			(unsigned int) (prefix->network >> 8) & 255,
			(unsigned int) prefix->network & 255,
			prefix->mask);
  return ((result < 0) || (result >= str_size)) ? -1 : result;
}

// ----- ip_string_to_prefix ----------------------------------------
/**
 *
 */
int ip_string_to_prefix(const char * str, char ** end_ptr,
			ip_pfx_t * prefix)
{
  unsigned long int digit;
  uint8_t num_digits= 4;

  if (prefix == NULL)
    return -1;
  prefix->network= 0;
  while (num_digits-- > 0) {
    digit= strtoul(str, end_ptr, 10);
    if ((digit > 255) || (str == *end_ptr))
      return -1;
    str= *end_ptr+1;
    prefix->network= (prefix->network << 8)+digit;
    if (**end_ptr == '/')
      break;
    if (**end_ptr != '.')
      return -1;
  }
  if (**end_ptr != '/')
    return -1;
  while (num_digits-- > 0)
    prefix->network<<= 8;
  digit= strtoul(str, end_ptr, 10);
  if (digit > 32)
    return -1;
  prefix->mask= digit;
  return 0;
}

// ----- ip_string_to_dest ------------------------------------------
/**
 * This function parses a string that contains the description of a
 * destination. The destination can be specified as an asterisk ('*'),
 * an IP prefix ('x.y/z') or an IP address ('a.b.c.d').
 *
 * If the destination is an asterisk, the function returns a
 * destination of type NET_DEST_ANY.
 *
 * If the destination is an IP prefix, the function returns a
 * destination of type NET_DEST_PREFIX.
 *
 * If the destination is an IP address, the function returns a
 * destination whose type is NET_DEST_ADDRESS.
 */
int ip_string_to_dest(const char * str, ip_dest_t * dest)
{
  char * endptr;

  if (!strcmp(str, "*")) {
    dest->type= NET_DEST_ANY;
    dest->prefix.mask= 0;
  } else if (!ip_string_to_prefix(str, &endptr, &dest->prefix) &&
	     (*endptr == 0)) {
    dest->type= NET_DEST_PREFIX;
  } else if (!ip_string_to_address(str, &endptr,
				   &dest->prefix.network) &&
	     (*endptr == 0)) {
    dest->type= NET_DEST_ADDRESS;
  } else {
    dest->type= NET_DEST_INVALID;
    return -1;
  }
  return 0;
}

//---------- ip_prefix_to_dest ----------------------------------------------
ip_dest_t ip_prefix_to_dest(ip_pfx_t prefix)
{
  ip_dest_t dest;
  if (prefix.mask == 32){
    dest.type= NET_DEST_ADDRESS;
    dest.addr= prefix.network;
  }
  else {
    dest.type= NET_DEST_PREFIX;
    dest.prefix= prefix;
  }
  return dest;
}

//---------- ip_address_to_dest ----------------------------------------------
ip_dest_t ip_address_to_dest(net_addr_t addr)
{
  ip_dest_t dest;
  dest.type= NET_DEST_ADDRESS;
  dest.addr= addr;
  return dest;
}


// ----- ip_dest_dump -----------------------------------------------
/**
 *
 */
void ip_dest_dump(gds_stream_t * stream, ip_dest_t dest)
{
  switch (dest.type) {
  case NET_DEST_ADDRESS:
    ip_address_dump(stream, dest.addr);
    break;
  case NET_DEST_PREFIX:
    ip_prefix_dump(stream, dest.prefix);
    break;
  case NET_DEST_ANY:
    stream_printf(stream, "*");
    break;
  default:
    stream_printf(stream, "???");
  }
}

// -----[ ip_prefix_cmp ]--------------------------------------------
/**
 * Compare two prefixes. The function makes sure that the network
 * part is masked before the comparison is done.
 *
 * Return value:
 *   -1  if P1 < P2
 *   0   if P1 == P2
 *   1   if P1 > P2
 */
int ip_prefix_cmp(ip_pfx_t * prefix1, ip_pfx_t * prefix2)
{
  net_addr_t mask;

  // Pointers are equal ?
  if (prefix1 == prefix2)
    return 0;
  
  // Compare mask length
  if (prefix1->mask < prefix2->mask) 
    return -1;
  else if (prefix1->mask > prefix2->mask)
    return  1;

  // Compare masked network part
  mask= _ip_address_mask(prefix1->mask);
  return _ip_address_cmp(prefix1->network & mask,
			 prefix2->network & mask);
}

// ----- ip_address_in_prefix ---------------------------------------
/**
 * Test if the address is in the prefix (i.e. if the address matches
 * the prefix).
 *
 * Return value:
 *   != 0  if address is in prefix
 *   0     otherwise
 */
int ip_address_in_prefix(net_addr_t addr, ip_pfx_t prefix)
{
  assert(prefix.mask <= 32);

  // Warning, shift on 32-bit int is only defined if operand in [0-31]
  // Thus, special case for 0 mask-length (match everything)
  if (prefix.mask == 0)
    return 1;

  return ((addr >> (32-prefix.mask)) ==
	  (prefix.network >> (32-prefix.mask)));
}

// ----- ip_prefix_in_prefix ----------------------------------------
/**
 * Test if P1 is more specific than P2.
 *
 * Return value:
 *   != 0  if P1 is in P2
 *   0     otherwise
 */
int ip_prefix_in_prefix(ip_pfx_t prefix1, ip_pfx_t prefix2)
{
  assert(prefix2.mask <= 32);

  // Warning, shift on 32-bit int is only defined if operand in [0-31]
  // Thus, special case for 0 mask-length (match everything)
  if (prefix2.mask == 0)
    return 1;

  // P1.masklen must be >= P2.masklen
  if (prefix1.mask < prefix2.mask)
    return 0;

  // Compare bits masked with less specific (P2)
  // Warning, shift on 32-bit int is only defined if operand in [0-31]
  return ((prefix1.network >> (32-prefix2.mask)) ==
          (prefix2.network >> (32-prefix2.mask)));
}

// ----- ip_prefix_ge_prefix ----------------------------------------
/**
 * Test if P1 matches P2 and its prefix length is greater than or
 * equal to L.
 */
int ip_prefix_ge_prefix(ip_pfx_t prefix1, ip_pfx_t prefix2,
			uint8_t mask)
{
  if (!ip_prefix_in_prefix(prefix1, prefix2))
    return 0;

  return (prefix1.mask >= mask);
}

// ----- ip_prefix_le_prefix ----------------------------------------
/**
 * Test if P1 is less or equal than P2.
 */
int ip_prefix_le_prefix(ip_pfx_t prefix1, ip_pfx_t prefix2,
			uint8_t mask)
{
  if (!ip_prefix_in_prefix(prefix1, prefix2))
    return 0;

  return (prefix1.mask <= mask);
}

// ----- ip_prefix_copy ---------------------------------------------
/**
 * Make a copy of the given IP prefix.
 */
ip_pfx_t * ip_prefix_copy(ip_pfx_t * prefix)
{
  ip_pfx_t * prefix_copy= (ip_pfx_t *) MALLOC(sizeof(ip_pfx_t));
  memcpy(prefix_copy, prefix, sizeof(ip_pfx_t));
  return prefix_copy;
}

// ----- ip_prefix_destroy ------------------------------------------
/**
 *
 */
void ip_prefix_destroy(ip_pfx_t ** prefix_ref)
{
  if (*prefix_ref != NULL) {
    FREE(*prefix_ref);
    *prefix_ref= NULL;
  }
}

// ----- ip_prefix_mask ---------------------------------------------
/**
 * This function masks the remaining bits of the prefix's address.
 */
void ip_prefix_mask(ip_pfx_t * prefix)
{
  prefix->network&= _ip_address_mask(prefix->mask);
}

