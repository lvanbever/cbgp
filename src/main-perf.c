// ==================================================================
// @(#)main-test.c
//
// Main source file for cbgp-test application.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/10/07
// $Id: main-perf.c,v 1.8 2009-04-02 19:11:21 bqu Exp $
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
#include <sys/time.h>

#include <libgds/hash_utils.h>
#include <libgds/str_util.h>

#include <api.h>
#include <bgp/attr/comm.h>
#include <bgp/attr/path.h>
#include <bgp/mrtd.h>
#include <bgp/route.h>

// -----[ test_rib_perf ]--------------------------------------------
/**
 * Test function (only available in EXPERIMENTAL mode).
 *
 * Current test function measures the number of nodes required to
 * store a complete RIB into a non-compact trie (radix-tree) or in a
 * compact trie (Patricia tree).
 */
#ifdef __EXPERIMENTAL__
/*int test_rib_perf(int argc, char * argv[])
{
  unsigned int index;
  bgp_routes_t * routes= mrtd_ascii_load_routes(NULL, pcArg);
  bgp_route_t * route;
  SRadixTree * rtree;
  gds_trie_t * trie;

  if (routes != NULL) {
    rtree= radix_tree_create(32, NULL);
    trie= trie_create(NULL);
    for (index= 0; index < routes_list_get_num(routes); index++) {
      route= (bgp_route_t *) routes->data[index];
      radix_tree_add(rtree, route->prefix.network,
		       route->prefix.mask, route);
      trie_insert(trie, route->prefix.network,
		  route->prefix.mask, route);
    }
    fprintf(stdout, "radix-tree: %d\n", radix_tree_num_nodes(rtree));
    fprintf(stdout, "trie: %d\n", trie_num_nodes(trie));
    radix_tree_destroy(&rtree);
    trie_destroy(&trie);
    routes_list_destroy(&routes);
    return 0;
  } else {
    fprintf(stderr, "could not load \"%s\"\n", pcArg);
    return -1;
  }
  }*/
#endif


/////////////////////////////////////////////////////////////////////
//
// BGP ATTRIBUTES HASH FUNCTION EVALUATION
//
/////////////////////////////////////////////////////////////////////

#define AS_PATH_STR_SIZE 1024

// -----[ _array_path_destroy ]--------------------------------------
static void _array_path_destroy(void * item, const void * ctx)
{
  bgp_path_t * path= (bgp_path_t *) item;
  path_destroy(&path);
}

// -----[ _array_path_compare ]--------------------------------------
static int _array_path_compare(const void * item1,
			       const void * item2,
			       unsigned int elt_size)
{
  bgp_path_t * path1= *((bgp_path_t **) item1);
  bgp_path_t * path2= *((bgp_path_t **) item2);

  return path_cmp(path1, path2);
}

// -----[ _array_comm_destroy ]--------------------------------------
static void _array_comm_destroy(void * item, const void * ctx)
{
}

// -----[ _array_comm_compare ]--------------------------------------
static int _array_comm_compare(const void * item1,
			       const void * item2,
			       unsigned int elt_size)
{
  bgp_comms_t * comms1= *((bgp_comms_t **) item1);
  bgp_comms_t * comms2= *((bgp_comms_t **) item2);

  return comms_cmp(comms1, comms2);
}


// -----[ path_to_buf ]----------------------------------------------
/**
 * Convert an AS-Path to a byte stream.
 */
static inline size_t path_to_buf(bgp_path_t * path, uint8_t * buf,
				 size_t size)
{
  unsigned int index, index2;
  bgp_path_seg_t * seg;
  uint8_t * buf_start= buf;

  for (index= 0; index < path_num_segments(path); index++) {
    seg= (bgp_path_seg_t *) path->data[index];
    *(buf++)= seg->type;
    for (index2= 0; index2 < seg->length; index2++) {
      *(buf++)= seg->asns[index2] & 255;
      *(buf++)= seg->asns[index2] >> 8;
    }
  }
  return buf-buf_start;
}

// -----[ comm_to_buff ]---------------------------------------------
/**
 * Convert a Communities to a byte stream.
 */
static inline size_t comm_to_buf(bgp_comms_t * comms, uint8_t * buf,
				 size_t size)
{
  unsigned int index;
  uint8_t * buf_start= buf;
  bgp_comm_t comm;

  if (comms != NULL) {
    for (index= 0; index < comms->num; index++) {
      comm= comms->values[index];
      *(buf++)= comm & 255;
      comm= comm >> 8;
      *(buf++)= comm & 255;
      comm= comm >> 8;
      *(buf++)= comm & 255;
      comm= comm >> 8;
      *(buf++)= comm;
    }
  }
  return buf-buf_start;
}

//
// The following hash function is from Bob Jenkins
//

/* The golden ratio: an arbitrary value */
#define JHASH_GOLDEN_RATIO  0x9e3779b9

/* NOTE: Arguments are modified. */
#define __jhash_mix(a, b, c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

/* The most generic version, hashes an arbitrary sequence
 * of bytes.  No alignment or length assumptions are made about
 * the input key.
 */
uint32_t jhash (void *key, uint32_t length, uint32_t initval)
{
  uint32_t a, b, c, len;
  uint8_t *k = key;

  len = length;
  a = b = JHASH_GOLDEN_RATIO;
  c = initval;

  while (len >= 12)
    {
      a +=
        (k[0] + ((uint32_t) k[1] << 8) + ((uint32_t) k[2] << 16) +
         ((uint32_t) k[3] << 24));
      b +=
        (k[4] + ((uint32_t) k[5] << 8) + ((uint32_t) k[6] << 16) +
         ((uint32_t) k[7] << 24));
      c +=
        (k[8] + ((uint32_t) k[9] << 8) + ((uint32_t) k[10] << 16) +
         ((uint32_t) k[11] << 24));

      __jhash_mix (a, b, c);

      k += 12;
      len -= 12;
    }

  c += length;
  switch (len)
    {
    case 11:
      c += ((uint32_t) k[10] << 24);
    case 10:
      c += ((uint32_t) k[9] << 16);
    case 9:
      c += ((uint32_t) k[8] << 8);
    case 8:
      b += ((uint32_t) k[7] << 24);
    case 7:
      b += ((uint32_t) k[6] << 16);
    case 6:
      b += ((uint32_t) k[5] << 8);
    case 5:
      b += k[4];
    case 4:
      a += ((uint32_t) k[3] << 24);
    case 3:
      a += ((uint32_t) k[2] << 16);
    case 2:
      a += ((uint32_t) k[1] << 8);
    case 1:
      a += k[0];
    };

  __jhash_mix (a, b, c);

  return c;
}

// -----[ path_strhash ]---------------------------------------------
static uint32_t path_strhash(const void * item, unsigned int hash_size)
{
  char str[AS_PATH_STR_SIZE];
  bgp_path_t * path= (bgp_path_t *) item;

  assert(path_to_string(path, 1,
			str, AS_PATH_STR_SIZE) < AS_PATH_STR_SIZE);
  return hash_utils_key_compute_string(str, hash_size) % hash_size;
}

// -----[ path_jhash]------------------------------------------------
static uint32_t path_jhash(const void * pItem, unsigned int hash_size)
{
  bgp_path_t * path= (bgp_path_t *) pItem;
  uint8_t auBuffer[1024];
  size_t tSize;

  tSize= path_to_buf(path, auBuffer, sizeof(auBuffer));
  return jhash(auBuffer, tSize, JHASH_GOLDEN_RATIO) % hash_size;
}

// -----[ comm_strhash ]---------------------------------------------
static uint32_t comm_strhash(const void * pItem, unsigned int hash_size)
{
  char acStr[AS_PATH_STR_SIZE];
  bgp_comms_t * pComm= (bgp_comms_t *) pItem;

  assert(comm_to_string(pComm, acStr, sizeof(acStr)) < AS_PATH_STR_SIZE);
  return hash_utils_key_compute_string(acStr, hash_size) % hash_size;;
}

// -----[ comm_hash_zebra ]------------------------------------------
static uint32_t comm_hash_zebra(const void * pItem, unsigned int hash_size)
{
  bgp_comms_t * pComm= (bgp_comms_t *) pItem;
  uint8_t auBuffer[1024];
  size_t tSize;
  uint32_t uKey= 0;

  tSize= comm_to_buf(pComm, auBuffer, sizeof(auBuffer));
  for (; tSize > 0; tSize--) {
    uKey+= auBuffer[tSize] & 255;
  }
  return uKey % hash_size;
}

// -----[ comm_jhash ]-----------------------------------------------
static uint32_t comm_jhash(const void * pItem, unsigned int hash_size)
{
  bgp_comms_t * pComm= (bgp_comms_t *) pItem;
  uint8_t auBuffer[1024];
  size_t tSize;

  tSize= comm_to_buf(pComm, auBuffer, sizeof(auBuffer));
  return jhash(auBuffer, tSize, JHASH_GOLDEN_RATIO) % hash_size;
}

typedef uint32_t (*FHashFunction)(const void * pItem, unsigned int);

typedef struct {
  FHashFunction fHashFunc;
  char *        name;
} hash_method_t;

hash_method_t PATH_HASH_METHODS[]= {
  {path_strhash, "Path-String"},
  {path_hash_zebra, "Path-Zebra"},
  {path_hash_OAT, "Path-OAT"},
  {path_jhash, "Path-Jenkins"},
};
#define PATH_HASH_METHODS_NUM sizeof(PATH_HASH_METHODS)/sizeof(PATH_HASH_METHODS[0])

hash_method_t COMM_HASH_METHODS[]= {
  {comm_strhash, "Comm-String"},
  {comm_hash_zebra, "Comm-Zebra"},
  /*{comm_hash_OAT, "Comm-OAT"},*/
  {comm_jhash, "Comm-Jenkins"},
};
#define COMM_HASH_METHODS_NUM sizeof(COMM_HASH_METHODS)/sizeof(COMM_HASH_METHODS[0])

typedef struct {
  hash_method_t * methods;
  uint8_t         num_methods;
  char *          name;
  ptr_array_t   * array;
} attr_info_t;

attr_info_t ATTR_INFOS[]= {
  {PATH_HASH_METHODS, PATH_HASH_METHODS_NUM, "Path", NULL},
  {COMM_HASH_METHODS, COMM_HASH_METHODS_NUM, "Comm", NULL},
};
#define ATTR_INFOS_NUM sizeof(ATTR_INFOS)/sizeof(ATTR_INFOS[0])

#ifdef HAVE_BGPDUMP
// -----[ _route_handler ]-------------------------------------------
/**
 * Handle routes loaded by the bgpdump library. Store the route's
 * attributes in the target arrays. Supported attributes are AS-Path
 * and Communities.
 */
static int _route_handler(int status, bgp_route_t * route,
			  net_addr_t peer_addr,
			  asn_t peer_asn, void * ctx)
{
  attr_info_t * attr_infos= (attr_info_t *) ctx;

  if (status != BGP_INPUT_STATUS_OK)
    return BGP_INPUT_SUCCESS;

  // AS-Path attribute
  ptr_array_add(attr_infos[0].array, &route->attr->path_ref);
  route->attr->path_ref= NULL;

  // Communities attribute
  ptr_array_add(attr_infos[1].array, &route->attr->comms);
  route->attr->comms= NULL;

  //route_destroy(&route);
  return BGP_INPUT_SUCCESS;
}
#endif /* HAVE_BGPDUMP */

// -----[ test_path_hash_perf ]--------------------------------------
int test_path_hash_perf(int argc, char * argv[])
{
  unsigned int index;
  struct timeval tp;
  double dStartTime;
  double dEndTime;
#define MAX_HASH_SIZE 65536
  unsigned int hash_size= MAX_HASH_SIZE;
  int aiHash[MAX_HASH_SIZE];
  uint32_t uKey;
  double dChiSquare, dExpected, dElement;
  uint32_t uDegreeFreedom;
  unsigned int attr_index;
  unsigned int hash_index;
  FHashFunction fHashFunc;

  if (argc < 3) {
    stream_printf(gdserr, "Error: incorrect number of arguments"
		  " (3 needed).\n");
    return -1;
  }

  if ((str_as_uint(argv[1], &hash_size) < 0) ||
      (hash_size > MAX_HASH_SIZE)) {
    stream_printf(gdserr, "Error: invalid hash size specified.\n");
    return -1;
  }

  ATTR_INFOS[0].array= ptr_array_create(ARRAY_OPTION_SORTED |
					 ARRAY_OPTION_UNIQUE,
					 _array_path_compare,
					_array_path_destroy,
					NULL);
  ATTR_INFOS[1].array= ptr_array_create(ARRAY_OPTION_SORTED |
					 ARRAY_OPTION_UNIQUE,
					 _array_comm_compare,
					_array_comm_destroy,
					NULL);

  // Load AS-Paths from specified BGP routing tables
  stream_printf(gdserr,
		"***** loading files *******************"
		"***************************************\n");
  while (argc-- > 2) {
    stream_printf(gdserr, "* %s...", argv[2]);
    stream_flush(gdserr);
#ifdef HAVE_BGPDUMP
    if (mrtd_binary_load(argv[2], _route_handler, ATTR_INFOS) != 0)
      stream_printf(gdserr, " KO :-(\n");
    else
      stream_printf(gdserr, " OK :-)\n");
#else
    stream_printf(gdserr, " KO :-(  /* not bound to libbgpdump */\n");
#endif /* HAVE_BGPDUMP */
    stream_flush(gdserr);
  }


  stream_printf(gdserr,
		"\n***** analysing data ******************"
		"***************************************\n");

  for (attr_index= 0; attr_index < ATTR_INFOS_NUM; attr_index++) {

    hash_method_t * methods= ATTR_INFOS[attr_index].methods;
    uint8_t num_methods= ATTR_INFOS[attr_index].num_methods;
    ptr_array_t * array= ATTR_INFOS[attr_index].array;

    stream_printf(gdserr, "***** Attribute [%s]\n",
		  ATTR_INFOS[attr_index].name);
    
    stream_printf(gdserr, "- number of different values: %d\n",
		  ptr_array_length(array));

    // Adjust hash size so that each expected count per bin is at
    // least 5 (this is a common rule of thumb for the adequacy of
    // using the Chi-2 goodness of fit statistic)
    if ((hash_size == 0) || (hash_size > ptr_array_length(array)/5))
      hash_size= ptr_array_length(array)/5;
    
    // Compute number of degrees of freedom for Pearson's Chi-2 test:
    //   number of bins - number of independent parameters fitted (1) - 1
    uDegreeFreedom= hash_size-2; 
    stream_printf(gdserr, "- degrees of freedom: %d\n", uDegreeFreedom);

    stream_printf(gdserr, "- number of hash functions: %d\n", num_methods);
    stream_printf(gdserr, "- hash table size         : %d\n", hash_size);
    
    // For various hash functions,
    // - measure time for computing hash value
    // - measure goodness of fit using Chi-2 statistic
    for (hash_index= 0; hash_index < num_methods; hash_index++) {
      
      fHashFunc= methods[hash_index].fHashFunc;
      
      memset(aiHash, 0, sizeof(aiHash));
      
      // Measure computation time
      stream_printf(gdserr, "(%d) method \"%s\"\n",
		    hash_index, methods[hash_index].name);
      assert(gettimeofday(&tp, NULL) >= 0);
      dStartTime= tp.tv_sec*1000000.0 + tp.tv_usec*1.0;

      for (index= 0; index < ptr_array_length(array); index++) {
	uKey= fHashFunc(array->data[index], hash_size);
	aiHash[uKey]++;
      }
      assert(gettimeofday(&tp, NULL) >= 0);
      dEndTime= tp.tv_sec*1000000.0 + tp.tv_usec*1.0;
      stream_printf(gdserr, "  - elapsed time   : %f s\n",
		    (dEndTime-dStartTime)/1000000.0);
      
      // Measure goodness of fit using Pearson's Chi-2 statistic
      dChiSquare= 0;
      dExpected= ptr_array_length(array)*1.0 / (hash_size*1.0);
      for (index= 0; index < hash_size; index++) {
	dElement= aiHash[index]*1.0 - dExpected;
	dChiSquare+= (dElement*dElement)/dExpected;
      }
      stream_printf(gdserr, "  - chi-2 statistic: %f\n", dChiSquare);
      
      // Need to compute p-value for the above statistic, based on
      // Chi-2 distribution with same number of degrees of freedom.
      // Question: how to decide if the test succeeds based on the
      // p-value ?
      
    }
  }

  //ptr_array_destroy(&arrayPath);
  //ptr_array_destroy(&arrayComm);

  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// MAIN PART
//
/////////////////////////////////////////////////////////////////////

// -----[ main ]-----------------------------------------------------
int main(int argc, char * argv[])
{
  libcbgp_init(argc, argv);
  libcbgp_banner();

  test_path_hash_perf(argc, argv);

  libcbgp_done();
  return EXIT_SUCCESS;
}
