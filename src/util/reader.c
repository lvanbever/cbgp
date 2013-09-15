// ==================================================================
// @(#)reader.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author David Hauweele (david.hauweele@student.umons.ac.be)
// @date 22/12/2008
// $Id: reader.c,v 1.1 2009-08-31 09:57:31 bqu Exp $
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
#include <stdio.h>

#include <libgds/memory.h>

#include <util/reader.h>

#ifdef HAVE_LIBZ
# include <zlib.h>
typedef gzFile FILE_TYPE;
# define FILE_OPEN(N,A) gzopen(N, A)
# define FILE_DOPEN(N,A) gzdopen(N,A)
# define FILE_CLOSE(F) gzclose(F)
# define FILE_GETS(F,B,L) gzgets(F, B, L)
# define FILE_GETC(F) gzgetc(F)
# define FILE_EOF(F) gzeof(F)
#else
typedef FILE * FILE_TYPE;
# define FILE_OPEN(N,A) fopen(N, A)
# define FILE_DOPEN(N,A) fdopen(N,A)
# define FILE_CLOSE(F) fclose(F)
# define FILE_GETS(F,B,L) fgets(B,L,F)
# define FILE_GETC(F) fgetc(F)
# define FILE_EOF(F) feof(F)
#endif

// -----[ reader_type_t ]--------------------------------------------
typedef enum {
  READER_FILE,
  READER_STREAM,
} reader_type_t;

// -----[ reader_t ]-------------------------------------------------
struct reader_t {
  reader_type_t type;
  void   (*close)  (reader_t * reader);
  void   (*destroy)(reader_t ** reader);
  int    (*eof)    (reader_t * reader);
  void   (*flush)  (reader_t * reader);
  char * (*gets)   (reader_t * reader, char * buf, size_t buf_size);
  union {
    FILE_TYPE file;
    FILE      * stream;
  };
};

static void _lrprf_close(reader_t * reader)
{
  FILE_CLOSE(reader->file);
  reader->file= NULL;
}

static void _lrprf_destroy(reader_t ** reader_ref)
{
  reader_t * reader= *reader_ref;
  if (reader == NULL)
    return;
  if (reader->file != NULL)
    FILE_CLOSE(reader->file);
  FREE(reader);
  *reader_ref= NULL;
}

static int _lrprf_eof(reader_t * reader)
{
  return FILE_EOF(reader->file);
}

static void _lrprf_flush(reader_t * reader)
{
  char ch;
  while (((ch= FILE_GETC(reader->file)) != '\n') && 
	 (ch >= 0));
}

static char * _lrprf_gets(reader_t * reader, char * buf,
			  size_t buf_size)
{
  return FILE_GETS(reader->file, buf, buf_size);
}

static void _lrprs_close(reader_t * reader)
{
  reader->stream= NULL;
}

static void _lrprs_destroy(reader_t ** reader_ref)
{
  reader_t * reader= *reader_ref;
  if (reader == NULL)
    return;
  FREE(reader);
  *reader_ref= NULL;
}

static int _lrprs_eof(reader_t * reader)
{
  return feof(reader->stream);
}

static void _lrprs_flush(reader_t * reader)
{
  char ch;
  while (((ch= fgetc(reader->stream)) != '\n') && 
	 (ch >= 0));
}

static char * _lrprs_gets(reader_t * reader, char * buf,
			  size_t buf_size)
{
  return fgets(buf, buf_size, reader->stream);
}

// -----[ file_reader ]----------------------------------------------
reader_t * file_reader(const char * filename)
{
  reader_t * reader= MALLOC(sizeof(reader_t));
  reader->type= READER_FILE;
  reader->file= FILE_OPEN(filename, "r");
  reader->close  = _lrprf_close;
  reader->destroy= _lrprf_destroy;
  reader->eof    = _lrprf_eof;
  reader->flush  = _lrprf_flush;
  reader->gets   = _lrprf_gets;
  return reader;
}

// -----[ stream_reader ]--------------------------------------------
reader_t * stream_reader(FILE * stream)
{
  reader_t * reader= MALLOC(sizeof(reader_t));
  reader->type= READER_STREAM;
  reader->stream= stream;
  reader->close  = _lrprs_close;
  reader->destroy= _lrprs_destroy;
  reader->eof    = _lrprs_eof;
  reader->flush  = _lrprs_flush;
  reader->gets   = _lrprs_gets;
  return reader;
}

// -----[ reader_close ]-------------------------------------------
void reader_close(reader_t * reader)
{
  reader->close(reader);
}

// -----[ reader_destroy ]-----------------------------------------
void reader_destroy(reader_t ** reader_ref)
{
  if (*reader_ref != NULL)
    (*reader_ref)->destroy(reader_ref);
}

// -----[ reader_eof ]---------------------------------------------
int reader_eof(reader_t * reader)
{
  return reader->eof(reader);
}

// -----[ reader_flush ]---------------------------------------------
void reader_flush(reader_t * reader)
{
  reader->flush(reader);
}

// -----[ reader_gets ]--------------------------------------------
char * reader_gets(reader_t * reader, char * buf, size_t buf_size)
{
  return reader->gets(reader, buf, buf_size);
}

