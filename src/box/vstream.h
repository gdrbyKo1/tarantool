#ifndef TARANTOOL_VSTREAM_H_INCLUDED
#define TARANTOOL_VSTREAM_H_INCLUDED
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

#include "diag.h"

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

struct vstream;
struct mpstream;
struct lua_State;
struct port;

struct vstream_vtab {
	void (*encode_array)(struct vstream *stream, uint32_t size);
	void (*encode_map)(struct vstream *stream, uint32_t size);
	void (*encode_uint)(struct vstream *stream, uint64_t num);
	void (*encode_int)(struct vstream *stream, int64_t num);
	void (*encode_float)(struct vstream *stream, float num);
	void (*encode_double)(struct vstream *stream, double num);
	void (*encode_strn)(struct vstream *stream, const char *str,
			    uint32_t len);
	void (*encode_nil)(struct vstream *stream);
	void (*encode_bool)(struct vstream *stream, bool val);
	void (*encode_enum)(struct vstream *stream, int64_t num,
			    const char *str);
	int (*encode_port)(struct vstream *stream, struct port *port);
	int (*encode_reply_array)(struct vstream *stream, uint32_t size,
				  uint8_t key, const char *str);
	int (*encode_reply_map)(struct vstream *stream, uint32_t size,
				uint8_t key, const char *str);
	void (*encode_array_commit)(struct vstream *stream, uint32_t id);
	void (*encode_reply_commit)(struct vstream *stream);
	void (*encode_map_commit)(struct vstream *stream);
};

struct vstream {
	/** Virtual function table. */
	const struct vstream_vtab *vtab;
	/** TODO: Write comment. */
	union {
		struct mpstream *mpstream;
		struct lua_State *L;
	};
	/** TODO: Write comment. */
	bool is_flatten;
};

void
mp_vstream_init(struct vstream *vstream, struct mpstream *mpstream);

void
lua_vstream_init(struct vstream *vstream, struct lua_State *L);

static inline void
vstream_encode_array(struct vstream *stream, uint32_t size)
{
	return stream->vtab->encode_array(stream, size);
}

static inline void
vstream_encode_map(struct vstream *stream, uint32_t size)
{
	return stream->vtab->encode_map(stream, size);
}

static inline void
vstream_encode_uint(struct vstream *stream, uint64_t num)
{
	return stream->vtab->encode_uint(stream, num);
}

static inline void
vstream_encode_int(struct vstream *stream, int64_t num)
{
	return stream->vtab->encode_int(stream, num);
}

static inline void
vstream_encode_float(struct vstream *stream, float num)
{
	return stream->vtab->encode_float(stream, num);
}

static inline void
vstream_encode_double(struct vstream *stream, double num)
{
	return stream->vtab->encode_double(stream, num);
}

static inline void
vstream_encode_strn(struct vstream *stream, const char *str, uint32_t len)
{
	return stream->vtab->encode_strn(stream, str, len);
}

static inline void
vstream_encode_str(struct vstream *stream, const char *str)
{
	return stream->vtab->encode_strn(stream, str, strlen(str));
}

static inline void
vstream_encode_nil(struct vstream *stream)
{
	return stream->vtab->encode_nil(stream);
}

static inline void
vstream_encode_bool(struct vstream *stream, bool val)
{
	return stream->vtab->encode_bool(stream, val);
}

static inline void
vstream_encode_enum(struct vstream *stream, int64_t num, const char *str)
{
	return stream->vtab->encode_enum(stream, num, str);
}

static inline int
vstream_encode_port(struct vstream *stream, struct port *port)
{
	return stream->vtab->encode_port(stream, port);
}

static inline int
vstream_encode_reply_array(struct vstream *stream, uint32_t size, uint8_t key,
			   const char *str)
{
	return stream->vtab->encode_reply_array(stream, size, key, str);
}

static inline int
vstream_encode_reply_map(struct vstream *stream, uint32_t size, uint8_t key,
			 const char *str)
{
	return stream->vtab->encode_reply_map(stream, size, key, str);
}

static inline void
vstream_encode_reply_commit(struct vstream *stream)
{
	return stream->vtab->encode_reply_commit(stream);
}

static inline void
vstream_encode_map_commit(struct vstream *stream)
{
	return stream->vtab->encode_map_commit(stream);
}

static inline void
vstream_encode_array_commit(struct vstream *stream, uint32_t id)
{
	return stream->vtab->encode_array_commit(stream, id);
}

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* TARANTOOL_VSTREAM_H_INCLUDED */
