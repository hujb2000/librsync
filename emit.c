/*=				       	-*- c-file-style: "bsd" -*-
 *
 * libhsync -- dynamic caching and delta update in HTTP
 * $Id$
 * 
 * Copyright (C) 2000 by Martin Pool <mbp@samba.org>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


                              /*
			       * They don't sleep anymore on the beach
			       */


#include "config.h"

#include <assert.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <sys/types.h>
#include <limits.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "hsync.h"
#include "command.h"
#include "protocol.h"
#include "trace.h"
#include "emit.h"
#include "netint.h"


#if 0
static int
_hs_fits_in_n8(size_t val)
{
    return val <= UINT8_MAX;
}


static int
_hs_fits_in_n16(size_t val)
{
    return val <= UINT16_MAX;
}


static int
_hs_fits_in_n32(size_t val)
{
    return val <= UINT32_MAX;
}


static int
_hs_int_len(off_t val)
{
    if (_hs_fits_in_n8(val))
	return 1;
    else if (_hs_fits_in_n16(val))
	return 2;
    else if (_hs_fits_in_n32(val))
	return 4;
    else {
	_hs_fatal("can't handle files this long yet");
    }
}
#endif


/*
 * Write the magic for the start of a delta.
 */
void
_hs_emit_delta_header(hs_stream_t *stream)
{
    _hs_squirt_n32(stream, HS_DELTA_MAGIC);
}



/* Write a LITERAL command. */
void
_hs_emit_literal_cmd(hs_stream_t *stream, int len)
{
    _hs_squirt_n8(stream, op_literal_n32);
    _hs_squirt_n32(stream, len);
}



#if 0
int _hs_emit_eof(hs_write_fn_t write_fn, void *write_priv,
		 UNUSED(hs_stats_t *stats))
{
    _hs_trace("Writing EOF");
    return _hs_write_netbyte(write_fn, write_priv, (uint8_t) op_eof);
}


int
_hs_emit_checksum_cmd(hs_write_fn_t write_fn, void *write_priv, size_t size)
{
     int ret;
     
     _hs_trace("Writing CHECKSUM(len=%d)", size);
     ret = _hs_write_netbyte(write_fn, write_priv,
			     (uint8_t) op_checksum_short);
     if (ret != 1)
	  return -1;

     assert(_hs_fits_in_short(size));
     ret = _hs_write_netshort(write_fn, write_priv, (uint16_t) size);
     if (ret != 2)
	  return -1;

     return 3;
}



int
_hs_emit_filesum(hs_write_fn_t write_fn, void *write_priv,
		 byte_t const *buf, size_t size)
{
     int ret;

     ret = _hs_emit_checksum_cmd(write_fn, write_priv, size);
     if (ret <= 0)
	  return -1;

     ret = _hs_write_loop(write_fn, write_priv, buf, size);
     if (ret != (int) size)
	  return -1;

     return 3 + size;
}


/* Emit the command header for literal data. */
int
_hs_emit_literal_cmd(hs_write_fn_t write_fn, void *write_priv, size_t size)
{
     int type;
     uint8_t cmd;
     
     _hs_trace("Writing LITERAL(len=%d)", size);

     if ((size >= 1)  &&  (size < (op_literal_last - op_literal_1))) {
	 cmd = (uint8_t) (op_literal_1 + size - 1);
	 return _hs_write_netbyte(write_fn, write_priv, cmd);
     }
     
     type = _hs_int_len(size);
     if (type == 1) {
	  cmd = (uint8_t) op_literal_byte;
     } else if (type == 2) {
	  cmd = (uint8_t) op_literal_short;
     } else if (type == 4) {
	 cmd = (uint8_t) op_literal_int;
     } else {
	 _hs_fatal("can't happen!");
     }

     if (_hs_write_netbyte(write_fn, write_priv, cmd) < 0)
	  return -1;

     return _hs_write_netvar(write_fn, write_priv, size, type);
}


int
_hs_send_literal(hs_write_fn_t write_fn,
		 void *write_priv,
		 int kind,
		 byte_t const *buf,
		 size_t amount)
{
    int ret;

    assert(amount > 0);
    
    _hs_trace("flush %d bytes of %s data",
	      (int) amount,
	      kind == op_kind_literal ? "literal" : "signature");

    if (kind == op_kind_literal) {
	if (_hs_emit_literal_cmd(write_fn, write_priv, amount) < 0)
	    return -1;
    } else {
	if (_hs_emit_signature_cmd(write_fn, write_priv, amount) < 0)
	    return -1;
    }

    ret = hs_must_write(write_fn, write_priv, buf, amount);
    assert(ret == (int)amount);
    return amount;
}


/* Emit the command header for signature data. */
int
_hs_emit_signature_cmd(hs_write_fn_t write_fn, void *write_priv,
		       size_t size)
{
     int type;
     uint8_t cmd;
     
     _hs_trace("Writing SIGNATURE(len=%d)", size);

     if ((size >= 1)  &&  (size < (op_signature_last - op_signature_1))) {
	 cmd = (uint8_t) (op_signature_1 + size - 1);
	  return _hs_write_netbyte(write_fn, write_priv, cmd);
     }
     
     type = _hs_int_len((long) size);
     if (type == 1) {
	  cmd = (uint8_t) op_signature_byte;
     } else if (type == 2) {
	  cmd = (uint8_t) op_signature_short;
     } else if (type == 4) {
	  cmd = (uint8_t) op_signature_int;
     } else {
	 _hs_fatal("can't happen!");
     }

     if (_hs_write_netbyte(write_fn, write_priv, cmd) < 0)
	  return -1;

     return _hs_write_netvar(write_fn, write_priv, size, type);
}


int
_hs_emit_copy(hs_write_fn_t write_fn, void *write_priv,
	      off_t offset, size_t length,
              hs_stats_t * stats)
{
    int ret;
    int len_type, off_type;
    int cmd;

    stats->copy_cmds++;
    stats->copy_bytes += length;

    _hs_trace("Writing COPY(off=%ld, len=%ld)", (long) offset, (long) length);
    len_type = _hs_int_len(length);
    off_type = _hs_int_len(offset);

    /* We cannot specify the offset as a byte, because that would so
       rarely be useful.  */
    if (off_type == 1)
	 off_type = 2;

    /* Make sure this formula lines up with the values in protocol.h */

    if (off_type == 2) {
	cmd = op_copy_short_byte;
    } else if (off_type == 4) {
	cmd = op_copy_int_byte;
    } else {
	_hs_fatal("can't pack offset %ld!", (long) offset);
    }

    if (len_type == 1) {
	cmd += 0;
    } else if (len_type == 2) {
	cmd += 1;
    } else if (len_type == 4) {
	cmd += 2;
    } else {
	 _hs_fatal("can't pack length %ld as a %d byte number",
		   (long) length, len_type);
    }

    ret = _hs_write_netbyte(write_fn, write_priv, (uint8_t) cmd);
    if (ret < 0) return -1;

    ret = _hs_write_netvar(write_fn, write_priv, offset, off_type);
    if (ret < 0) return -1;

    ret = _hs_write_netvar(write_fn, write_priv, length, len_type);
    if (ret < 0) return -1;

    return 1;
}
#endif
