// ==================================================================
// @(#)regex.c
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/09/2004
// $Id: regex.c,v 1.5 2009-03-24 16:30:03 bqu Exp $
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

#include <config.h>

/* 
 * Simple regex wrapper based on the pcre library. It's mainly 
 * conceived for very simple matching of regexp.
 */

#include <stdio.h>
#include <string.h>

#include <libgds/memory.h>
#include <libgds/stream.h>

#include <util/regex.h>

#ifdef HAVE_LIBPCRE

#ifdef HAVE_PCRE_PCRE_H
#  include <pcre/pcre.h>
#else
#  include <pcre.h>
#endif

struct RegEx {
  pcre * pRegEx;
  unsigned int uMaxResult;
  int * iVectorResult;
  int isFirstSearch;
  int iNumResult;
  int iNbrResult;
  char * sResult;
};

// ----- regex_compile -----------------------------------------------
/**
 *
 *
 */
SRegEx * regex_init(const char * pattern, const unsigned int uMaxResult)
{
  SRegEx * pRegEx = MALLOC(sizeof(SRegEx));

  pcre *	RegEx;
  const char *	sError;
  int		iErrorOffset;
  
  RegEx = pcre_compile(pattern, 0, 
			  &sError, &iErrorOffset,
			  NULL);
  if (pRegEx == NULL) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG,
	      "regex_compile> PCRE compilation failed at offset"
	      "%d : %s\n", iErrorOffset, sError);
    FREE(pRegEx);
    pRegEx = NULL;
  } else {
    pRegEx->pRegEx = RegEx;
    pRegEx->uMaxResult = uMaxResult;
    pRegEx->iVectorResult = NULL;
    pRegEx->isFirstSearch = 0;
    pRegEx->iNumResult = 0;
    pRegEx->iNbrResult = 0;
    pRegEx->sResult = NULL;
  }
  return pRegEx;
}

void regex_reinit(SRegEx * pRegEx)
{
  pRegEx->iNbrResult = 0;
  pRegEx->iNbrResult = 0;
  if (pRegEx->sResult != NULL)
    FREE(pRegEx->sResult);
    pRegEx->sResult = NULL;
  pRegEx->isFirstSearch = 0;
  if (pRegEx->iVectorResult != NULL)
    FREE(pRegEx->iVectorResult);
    pRegEx->iVectorResult = NULL;
}

// ----- regex_next_search -------------------------------------------
/**
 *
 *
 */
void regex_next_search(SRegEx * pRegEx, const char * sExp)
{
  int options = 0;                 /* Normally no options */
  int start_offset = pRegEx->iVectorResult[1];   /* Start at end of previous match */

  pRegEx->iNumResult = 0;
  pRegEx->iNbrResult = 0;
  
  /* If the previous match was for an empty string, we are finished if we are
  at the end of the subject. Otherwise, arrange to run another match at the
  same point to see if a non-empty match can be found. */
  if (pRegEx->iVectorResult[0] == pRegEx->iVectorResult[1]) {
    if (pRegEx->iVectorResult[0] == strlen(sExp)) return;
      options = PCRE_NOTEMPTY | PCRE_ANCHORED;
  }

  /* Run the next matching operation */
  pRegEx->iNbrResult = pcre_exec(
    pRegEx->pRegEx,                   /* the compiled pattern */
    NULL,                 /* no extra data - we didn't study the pattern */
    sExp,              /* the subject string */
    strlen(sExp),       /* the length of the subject */
    start_offset,         /* starting offset in the subject */
    options,              /* options */
    pRegEx->iVectorResult,              /* output vector for substring information */
    (pRegEx->uMaxResult+1)*3);           /* number of elements in the output vector */

  /* This time, a result of NOMATCH isn't an error. If the value in "options"
  is zero, it just means we have found all possible matches, so the loop ends.
  Otherwise, it means we have failed to find a non-empty-string match at a
  point where there was a previous empty-string match. In this case, we do what
  Perl does: advance the matching position by one, and continue. We do this by
  setting the "end of previous match" offset, because that is picked up at the
  top of the loop as the point at which to start again. */
  if (pRegEx->iNbrResult == PCRE_ERROR_NOMATCH) {
    if (options == 0) return;
    pRegEx->iVectorResult[1] = start_offset + 1;
    regex_next_search(pRegEx, sExp);    /* Go round the loop again */
  } else {
    /* Other matching errors are not recoverable. */
    if (pRegEx->iNbrResult < 0) {
      STREAM_DEBUG(STREAM_LEVEL_DEBUG,
		"regex_get_next_result> Matching error %d\n", pRegEx->iNbrResult);
    }
    /* The match succeeded, but the output vector wasn't big enough. */
    /* if it does not happens before when calling regex_search, it would normally not happen here!*/
    if (pRegEx->iNbrResult == 0) {
      STREAM_DEBUG(STREAM_LEVEL_DEBUG,
		"regex_get_next_result> Match succeeded but no enough place to store results\n");
    }
  }

}
void regex_first_search(SRegEx * pRegEx, const char * sExp)
{
  pRegEx->iNbrResult = pcre_exec(pRegEx->pRegEx, NULL, sExp, strlen(sExp), 0, 0,
				pRegEx->iVectorResult, 
				(pRegEx->uMaxResult+1)*3);

  //printf("exp : %s\n", sExp);
  if (pRegEx->iNbrResult < 0) {
    switch (pRegEx->iNbrResult) {
      //No match of the pRegEx in sExp
      case PCRE_ERROR_NOMATCH:
	pRegEx->iNbrResult = 0;
       break;
    /* PCRE_ERROR_NULL
     * PCRE_ERROR_BADOPTION
     * PCRE_ERROR_BADMAGIC
     * PCRE_ERROR_UNKNOWN_NODE 
     * PCRE_ERROR_NOMEMORY
     * PCRE_ERROR_NOSUBSTRING
     * PCRE_ERROR_MATCHLIMIT
     * PCRE_ERROR_CALLOUT
     * PCRE_ERROR_BADUTF8
     * PCRE_ERROR_BADUTF8_OFFSET
     * PCRE_ERROR_PARTIAL
     * PCRE_ERROR_BAD_PARTIAL
     * PCRE_ERROR_INTERNAL
     * PCRE_ERROR_BADCOUNT
     */
      default: 
	STREAM_DEBUG(STREAM_LEVEL_DEBUG,
		  "regex_exec>Matching error %d\n", pRegEx->iNbrResult);
    }
    FREE(pRegEx->iVectorResult);
    pRegEx->iVectorResult = NULL;
  }
}



// ----- regex_exec --------------------------------------------------
/**
 * returns the number of matching in the expression
 *
 */
int regex_search(SRegEx * pRegEx, const char * sExp)
{
  if (pRegEx != NULL) {
    if (pRegEx->iVectorResult == NULL)
      pRegEx->iVectorResult = MALLOC( (sizeof(int)*(pRegEx->uMaxResult+1)) * 3);
  
    if (pRegEx->sResult != NULL) {
      FREE(pRegEx->sResult);
      pRegEx->sResult = NULL;
    }

    if (pRegEx->isFirstSearch == 0) {
      pRegEx->isFirstSearch = 1;
      regex_first_search(pRegEx, sExp);
    } else 
      regex_next_search(pRegEx, sExp);
  }
  //printf("NbrResult : %d\n", pRegEx->iNbrResult);
  return pRegEx->iNbrResult;
}

// ----- regex_copy_match_string -------------------------------------
/**
 *
 *
 */
char * regex_copy_match_string(const char * sOffsetExp, const uint32_t iMatchedStringLen)
{
  char * sMatchedString;

  sMatchedString = MALLOC(iMatchedStringLen+1);
  strncpy(sMatchedString, sOffsetExp, iMatchedStringLen);
  sMatchedString[iMatchedStringLen] = '\0';

  return sMatchedString;
}

// ----- regex_get_result --------------------------------------------
/**
 *
 * 
 *
 */
char * regex_get_result(SRegEx * pRegEx, const char * sExp)
{
  int iMatchedStringLen;
  
  if (pRegEx == NULL || pRegEx->iNumResult >= pRegEx->iNbrResult)
    return NULL;


  iMatchedStringLen = pRegEx->iVectorResult[pRegEx->iNumResult*2+1] - pRegEx->iVectorResult[pRegEx->iNumResult*2];
  pRegEx->sResult = regex_copy_match_string(sExp+pRegEx->iVectorResult[pRegEx->iNumResult*2], iMatchedStringLen);

  pRegEx->iNumResult++;

  return pRegEx->sResult;
}

// ----- regex_finalize ----------------------------------------------
/**
 *
 *
 */
void regex_finalize(SRegEx ** pRegEx)
{
  if (*pRegEx != NULL) {
    if ((*pRegEx)->pRegEx != NULL)
      free((*pRegEx)->pRegEx);
    if ((*pRegEx)->iVectorResult != NULL)
      FREE((*pRegEx)->iVectorResult);
    if ((*pRegEx)->sResult != NULL)
      FREE((*pRegEx)->sResult);
    FREE(*pRegEx);
    *pRegEx = NULL;
  }
}

#endif /* HAVE_LIBPCRE */

