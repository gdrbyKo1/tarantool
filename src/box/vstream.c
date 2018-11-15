/*
 * Copyright 2010-2018, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "vstream.h"
#include "mpstream.h"
#include "iproto_constants.h"
#include "port.h"
#include "xrow.h"
#include "lua/utils.h"
#include "lua/tuple.h"

void
mp_vstream_encode_array(struct vstream *stream, uint32_t size)
{
	mpstream_encode_array(stream->mpstream, size);
}

void
mp_vstream_encode_map(struct vstream *stream, uint32_t size)
{
	mpstream_encode_map(stream->mpstream, size);
}

void
mp_vstream_encode_uint(struct vstream *stream, uint64_t num)
{
	mpstream_encode_uint(stream->mpstream, num);
}

void
mp_vstream_encode_int(struct vstream *stream, int64_t num)
{
	mpstream_encode_int(stream->mpstream, num);
}

void
mp_vstream_encode_float(struct vstream *stream, float num)
{
	mpstream_encode_float(stream->mpstream, num);
}

void
mp_vstream_encode_double(struct vstream *stream, double num)
{
	mpstream_encode_double(stream->mpstream, num);
}

void
mp_vstream_encode_strn(struct vstream *stream, const char *str, uint32_t len)
{
	mpstream_encode_strn(stream->mpstream, str, len);
}

void
mp_vstream_encode_nil(struct vstream *stream)
{
	mpstream_encode_nil(stream->mpstream);
}

void
mp_vstream_encode_bool(struct vstream *stream, bool val)
{
	mpstream_encode_bool(stream->mpstream, val);
}

int
mp_vstream_encode_port(struct vstream *stream, struct port *port)
{
	mpstream_flush(stream->mpstream);
	/*
	 * Just like SELECT, SQL uses output format compatible
	 * with Tarantool 1.6
	 */
	if (port_dump_msgpack_16(port, stream->mpstream->ctx) < 0) {
		/* Failed port dump destroyes the port. */
		return -1;
	}
	mpstream_reset(stream->mpstream);
	return 0;
}

int
mp_vstream_encode_reply_array(struct vstream *stream, uint32_t size,
			      uint8_t key, const char *str)
{
	(void)str;
	uint8_t type = 0xdd;
	char *pos = mpstream_reserve(stream->mpstream, IPROTO_KEY_HEADER_LEN);
	if (pos == NULL) {
		diag_set(OutOfMemory, IPROTO_KEY_HEADER_LEN,
			 "mpstream_reserve", "pos");
		return -1;
	}
	pos = mp_store_u8(pos, key);
	pos = mp_store_u8(pos, type);
	pos = mp_store_u32(pos, size);
	mpstream_advance(stream->mpstream, IPROTO_KEY_HEADER_LEN);
	return 0;
}

int
mp_vstream_encode_reply_map(struct vstream *stream, uint32_t size, uint8_t key,
			    const char *str)
{
	(void)str;
	uint8_t type = 0xdf;
	char *pos = mpstream_reserve(stream->mpstream, IPROTO_KEY_HEADER_LEN);
	if (pos == NULL) {
		diag_set(OutOfMemory, IPROTO_KEY_HEADER_LEN,
			 "mpstream_reserve", "pos");
		return -1;
	}
	pos = mp_store_u8(pos, key);
	pos = mp_store_u8(pos, type);
	pos = mp_store_u32(pos, size);
	mpstream_advance(stream->mpstream, IPROTO_KEY_HEADER_LEN);
	return 0;
}

void
mp_vstream_encode_enum(struct vstream *stream, int64_t num, const char *str)
{
	(void)str;
	if (num < 0)
		mpstream_encode_int(stream->mpstream, num);
	else
		mpstream_encode_uint(stream->mpstream, num);
}

void
mp_vstream_encode_reply_commit(struct vstream *stream)
{
	(void)stream;
}

void
mp_vstream_encode_map_commit(struct vstream *stream)
{
	(void)stream;
}

void
mp_vstream_encode_array_commit(struct vstream *stream, uint32_t id)
{
	(void)stream;
	(void)id;
}

const struct vstream_vtab mp_vstream_vtab = {
	/** encode_array = */ mp_vstream_encode_array,
	/** encode_map = */ mp_vstream_encode_map,
	/** encode_uint = */ mp_vstream_encode_uint,
	/** encode_int = */ mp_vstream_encode_int,
	/** encode_float = */ mp_vstream_encode_float,
	/** encode_double = */ mp_vstream_encode_double,
	/** encode_strn = */ mp_vstream_encode_strn,
	/** encode_nil = */ mp_vstream_encode_nil,
	/** encode_bool = */ mp_vstream_encode_bool,
	/** encode_enum = */ mp_vstream_encode_enum,
	/** encode_port = */ mp_vstream_encode_port,
	/** encode_reply_array = */ mp_vstream_encode_reply_array,
	/** encode_reply_map = */ mp_vstream_encode_reply_map,
	/** encode_array_commit = */ mp_vstream_encode_array_commit,
	/** encode_reply_commit = */ mp_vstream_encode_reply_commit,
	/** encode_map_commit = */ mp_vstream_encode_map_commit,
};

void
mp_vstream_init(struct vstream *vstream, struct mpstream *mpstream)
{
	vstream->vtab = &mp_vstream_vtab;
	vstream->mpstream = mpstream;
	vstream->is_flatten = false;
}

void
lua_vstream_encode_array(struct vstream *stream, uint32_t size)
{
	lua_createtable(stream->L, size, 0);
}

void
lua_vstream_encode_map(struct vstream *stream, uint32_t size)
{
	(void)size;
	lua_newtable(stream->L);
}

void
lua_vstream_encode_uint(struct vstream *stream, uint64_t num)
{
	luaL_pushuint64(stream->L, num);
}

void
lua_vstream_encode_int(struct vstream *stream, int64_t num)
{
	luaL_pushint64(stream->L, num);
}

void
lua_vstream_encode_float(struct vstream *stream, float num)
{
	lua_pushnumber(stream->L, num);
}

void
lua_vstream_encode_double(struct vstream *stream, double num)
{
	lua_pushnumber(stream->L, num);
}

void
lua_vstream_encode_strn(struct vstream *stream, const char *str, uint32_t len)
{
	lua_pushlstring(stream->L, str, len);
}

void
lua_vstream_encode_nil(struct vstream *stream)
{
	lua_pushnil(stream->L);
}

void
lua_vstream_encode_bool(struct vstream *stream, bool val)
{
	lua_pushboolean(stream->L, val);
}

int
lua_vstream_encode_port(struct vstream *stream, struct port *port)
{
	if (port_dump_lua(port, stream->L) < 0)
		return -1;
	return 0;
}

int
lua_vstream_encode_reply_array(struct vstream *stream, uint32_t size,
			      uint8_t key, const char *str)
{
	(void)key;
	if (stream->is_flatten)
		return 0;
	lua_createtable(stream->L, size, 0);
	lua_setfield(stream->L, -2, str);
	lua_getfield(stream->L, -1, str);
	return 0;
}

int
lua_vstream_encode_reply_map(struct vstream *stream, uint32_t size, uint8_t key,
			    const char *str)
{
	(void)size;
	(void)key;
	if (stream->is_flatten)
		return 0;
	lua_newtable(stream->L);
	lua_setfield(stream->L, -2, str);
	lua_getfield(stream->L, -1, str);
	return 0;
}

void
lua_vstream_encode_enum(struct vstream *stream, int64_t num, const char *str)
{
	(void)num;
	lua_pushlstring(stream->L, str, strlen(str));
}

void
lua_vstream_encode_reply_commit(struct vstream *stream)
{
	if(!stream->is_flatten)
		lua_pop(stream->L, 1);
}

void
lua_vstream_encode_map_commit(struct vstream *stream)
{
	size_t length;
	const char *key = lua_tolstring(stream->L, -2, &length);
	lua_setfield(stream->L, -3, key);
	lua_pop(stream->L, 1);
}

void
lua_vstream_encode_array_commit(struct vstream *stream, uint32_t id)
{
	lua_rawseti(stream->L, -2, id + 1);
}

const struct vstream_vtab lua_vstream_vtab = {
	/** encode_array = */ lua_vstream_encode_array,
	/** encode_map = */ lua_vstream_encode_map,
	/** encode_uint = */ lua_vstream_encode_uint,
	/** encode_int = */ lua_vstream_encode_int,
	/** encode_float = */ lua_vstream_encode_float,
	/** encode_double = */ lua_vstream_encode_double,
	/** encode_strn = */ lua_vstream_encode_strn,
	/** encode_nil = */ lua_vstream_encode_nil,
	/** encode_bool = */ lua_vstream_encode_bool,
	/** encode_enum = */ lua_vstream_encode_enum,
	/** encode_port = */ lua_vstream_encode_port,
	/** encode_reply_array = */ lua_vstream_encode_reply_array,
	/** encode_reply_map = */ lua_vstream_encode_reply_map,
	/** encode_array_commit = */ lua_vstream_encode_array_commit,
	/** encode_reply_commit = */ lua_vstream_encode_reply_commit,
	/** encode_map_commit = */ lua_vstream_encode_map_commit,
};

void
lua_vstream_init(struct vstream *vstream, struct lua_State *L)
{
	vstream->vtab = &lua_vstream_vtab;
	vstream->L = L;
	vstream->is_flatten = false;
}
