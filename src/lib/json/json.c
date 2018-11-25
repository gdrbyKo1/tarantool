/*
 * Copyright 2010-2018 Tarantool AUTHORS: please see AUTHORS file.
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
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "json.h"
#include "third_party/PMurHash.h"
#include <ctype.h>
#include <stdbool.h>
#include <unicode/uchar.h>
#include <unicode/utf8.h>
#include "trivia/util.h"

/**
 * Read a single symbol from a string starting from an offset.
 * @param lexer JSON path lexer.
 * @param[out] UChar32 Read symbol.
 *
 * @retval   0 Success.
 * @retval > 0 1-based position of a syntax error.
 */
static inline int
json_read_symbol(struct json_lexer *lexer, UChar32 *out)
{
	if (lexer->offset == lexer->src_len) {
		*out = U_SENTINEL;
		return lexer->symbol_count + 1;
	}
	U8_NEXT(lexer->src, lexer->offset, lexer->src_len, *out);
	if (*out == U_SENTINEL)
		return lexer->symbol_count + 1;
	++lexer->symbol_count;
	return 0;
}

/**
 * Rollback one symbol offset.
 * @param lexer JSON path lexer.
 * @param offset Offset to the previous symbol.
 */
static inline void
json_revert_symbol(struct json_lexer *lexer, int offset)
{
	lexer->offset = offset;
	--lexer->symbol_count;
}

/** Fast forward when it is known that a symbol is 1-byte char. */
static inline void
json_skip_char(struct json_lexer *lexer)
{
	++lexer->offset;
	++lexer->symbol_count;
}

/** Get a current symbol as a 1-byte char. */
static inline char
json_current_char(const struct json_lexer *lexer)
{
	return *(lexer->src + lexer->offset);
}

/**
 * Parse string identifier in quotes. Lexer either stops right
 * after the closing quote, or returns an error position.
 * @param lexer JSON path lexer.
 * @param[out] token JSON token to store result.
 * @param quote_type Quote by that a string must be terminated.
 *
 * @retval   0 Success.
 * @retval > 0 1-based position of a syntax error.
 */
static inline int
json_parse_string(struct json_lexer *lexer, struct json_token *token,
		  UChar32 quote_type)
{
	assert(lexer->offset < lexer->src_len);
	assert(quote_type == json_current_char(lexer));
	/* The first symbol is always char  - ' or ". */
	json_skip_char(lexer);
	int str_offset = lexer->offset;
	UChar32 c;
	int rc;
	while ((rc = json_read_symbol(lexer, &c)) == 0) {
		if (c == quote_type) {
			int len = lexer->offset - str_offset - 1;
			if (len == 0)
				return lexer->symbol_count;
			token->key.type = JSON_TOKEN_STR;
			token->key.str = lexer->src + str_offset;
			token->key.len = len;
			return 0;
		}
	}
	return rc;
}

/**
 * Parse digit sequence into integer until non-digit is met.
 * Lexer stops right after the last digit.
 * @param lexer JSON lexer.
 * @param[out] token JSON token to store result.
 *
 * @retval   0 Success.
 * @retval > 0 1-based position of a syntax error.
 */
static inline int
json_parse_integer(struct json_lexer *lexer, struct json_token *token)
{
	const char *end = lexer->src + lexer->src_len;
	const char *pos = lexer->src + lexer->offset;
	assert(pos < end);
	int len = 0;
	uint64_t value = 0;
	char c = *pos;
	if (! isdigit(c))
		return lexer->symbol_count + 1;
	do {
		value = value * 10 + c - (int)'0';
		++len;
	} while (++pos < end && isdigit((c = *pos)));
	lexer->offset += len;
	lexer->symbol_count += len;
	token->key.type = JSON_TOKEN_NUM;
	token->key.num = value;
	return 0;
}

/**
 * Check that a symbol can be part of a JSON path not inside
 * ["..."].
 */
static inline bool
json_is_valid_identifier_symbol(UChar32 c)
{
	return u_isUAlphabetic(c) || c == (UChar32)'_' || u_isdigit(c);
}

/**
 * Parse identifier out of quotes. It can contain only alphas,
 * digits and underscores. And can not contain digit at the first
 * position. Lexer is stoped right after the last non-digit,
 * non-alpha and non-underscore symbol.
 * @param lexer JSON lexer.
 * @param[out] token JSON token to store result.
 *
 * @retval   0 Success.
 * @retval > 0 1-based position of a syntax error.
 */
static inline int
json_parse_identifier(struct json_lexer *lexer, struct json_token *token)
{
	assert(lexer->offset < lexer->src_len);
	int str_offset = lexer->offset;
	UChar32 c;
	int rc = json_read_symbol(lexer, &c);
	if (rc != 0)
		return rc;
	/* First symbol can not be digit. */
	if (!u_isalpha(c) && c != (UChar32)'_')
		return lexer->symbol_count;
	int last_offset = lexer->offset;
	while ((rc = json_read_symbol(lexer, &c)) == 0) {
		if (! json_is_valid_identifier_symbol(c)) {
			json_revert_symbol(lexer, last_offset);
			break;
		}
		last_offset = lexer->offset;
	}
	token->key.type = JSON_TOKEN_STR;
	token->key.str = lexer->src + str_offset;
	token->key.len = lexer->offset - str_offset;
	return 0;
}

int
json_lexer_next_token(struct json_lexer *lexer, struct json_token *token)
{
	if (lexer->offset == lexer->src_len) {
		token->key.type = JSON_TOKEN_END;
		return 0;
	}
	UChar32 c;
	int last_offset = lexer->offset;
	int rc = json_read_symbol(lexer, &c);
	if (rc != 0)
		return rc;
	switch(c) {
	case (UChar32)'[':
		/* Error for '[\0'. */
		if (lexer->offset == lexer->src_len)
			return lexer->symbol_count;
		c = json_current_char(lexer);
		if (c == '"' || c == '\'')
			rc = json_parse_string(lexer, token, c);
		else
			rc = json_parse_integer(lexer, token);
		if (rc != 0)
			return rc;
		/*
		 * Expression, started from [ must be finished
		 * with ] regardless of its type.
		 */
		if (lexer->offset == lexer->src_len ||
		    json_current_char(lexer) != ']')
			return lexer->symbol_count + 1;
		/* Skip ] - one byte char. */
		json_skip_char(lexer);
		return 0;
	case (UChar32)'.':
		if (lexer->offset == lexer->src_len)
			return lexer->symbol_count + 1;
		return json_parse_identifier(lexer, token);
	default:
		json_revert_symbol(lexer, last_offset);
		return json_parse_identifier(lexer, token);
	}
}

/** Compare JSON tokens keys. */
static int
json_token_key_cmp(const struct json_token *a, const struct json_token *b)
{
	if (a->key.type != b->key.type)
		return a->key.type - b->key.type;
	int ret = 0;
	if (a->key.type == JSON_TOKEN_STR) {
		if (a->key.len != b->key.len)
			return a->key.len - b->key.len;
		ret = memcmp(a->key.str, b->key.str, a->key.len);
	} else if (a->key.type == JSON_TOKEN_NUM) {
		ret = a->key.num - b->key.num;
	} else {
		unreachable();
	}
	return ret;
}

/**
 * Compare hash records of two json tree nodes. Return 0 if equal.
 */
static inline int
mh_json_cmp(const struct json_token *a, const struct json_token *b)
{
	if (a->parent != b->parent)
		return a->parent - b->parent;
	return json_token_key_cmp(a, b);
}

#define MH_SOURCE 1
#define mh_name _json
#define mh_key_t struct json_token **
#define mh_node_t struct json_token *
#define mh_arg_t void *
#define mh_hash(a, arg) ((*a)->rolling_hash)
#define mh_hash_key(a, arg) ((*a)->rolling_hash)
#define mh_cmp(a, b, arg) (mh_json_cmp((*a), (*b)))
#define mh_cmp_key(a, b, arg) mh_cmp(a, b, arg)
#include "salad/mhash.h"

static const uint32_t hash_seed = 13U;

/** Compute the hash value of a JSON token. */
static uint32_t
json_token_hash(struct json_token *token, uint32_t seed)
{
	uint32_t h = seed;
	uint32_t carry = 0;
	const void *data;
	uint32_t data_size;
	if (token->key.type == JSON_TOKEN_STR) {
		data = token->key.str;
		data_size = token->key.len;
	} else if (token->key.type == JSON_TOKEN_NUM) {
		data = &token->key.num;
		data_size = sizeof(token->key.num);
	} else {
		unreachable();
	}
	PMurHash32_Process(&h, &carry, data, data_size);
	return PMurHash32_Result(h, carry, data_size);
}

int
json_tree_create(struct json_tree *tree)
{
	memset(tree, 0, sizeof(struct json_tree));
	tree->root.rolling_hash = hash_seed;
	tree->root.key.type = JSON_TOKEN_END;
	tree->hash = mh_json_new();
	return tree->hash == NULL ? -1 : 0;
}

static void
json_token_destroy(struct json_token *token)
{
	free(token->children);
}

void
json_tree_destroy(struct json_tree *tree)
{
	assert(tree->hash != NULL);
	json_token_destroy(&tree->root);
	mh_json_delete(tree->hash);
}

struct json_token *
json_tree_lookup(struct json_tree *tree, struct json_token *parent,
		 struct json_token *token)
{
	if (parent == NULL)
		parent = &tree->root;
	struct json_token *ret = NULL;
	if (token->key.type == JSON_TOKEN_STR) {
		struct json_token key = *token;
		key.rolling_hash = json_token_hash(token, parent->rolling_hash);
		key.parent = parent;
		token = &key;
		mh_int_t id = mh_json_find(tree->hash, &token, NULL);
		if (unlikely(id == mh_end(tree->hash)))
			return NULL;
		struct json_token **entry = mh_json_node(tree->hash, id);
		assert(entry == NULL || (*entry)->parent == parent);
		return entry != NULL ? *entry : NULL;
	} else if (token->key.type == JSON_TOKEN_NUM) {
		uint32_t idx =  token->key.num - 1;
		ret = idx < parent->child_size ? parent->children[idx] : NULL;
	} else {
		unreachable();
	}
	return ret;
}

int
json_tree_add(struct json_tree *tree, struct json_token *parent,
	      struct json_token *token)
{
	if (parent == NULL)
		parent = &tree->root;
	uint32_t rolling_hash =
	       json_token_hash(token, parent->rolling_hash);
	assert(json_tree_lookup(tree, parent, token) == NULL);
	uint32_t insert_idx = (token->key.type == JSON_TOKEN_NUM) ?
			      (uint32_t)token->key.num - 1 :
			      parent->child_size;
	if (insert_idx >= parent->child_size) {
		uint32_t new_size =
			parent->child_size == 0 ? 1 : 2 * parent->child_size;
		while (insert_idx >= new_size)
			new_size *= 2;
		struct json_token **children =
			realloc(parent->children, new_size*sizeof(void *));
		if (unlikely(children == NULL))
			return -1;
		memset(children + parent->child_size, 0,
		       (new_size - parent->child_size)*sizeof(void *));
		parent->children = children;
		parent->child_size = new_size;
	}
	assert(parent->children[insert_idx] == NULL);
	parent->children[insert_idx] = token;
	parent->child_count = MAX(parent->child_count, insert_idx + 1);
	token->sibling_idx = insert_idx;
	token->rolling_hash = rolling_hash;
	token->parent = parent;

	const struct json_token **key =
		(const struct json_token **)&token;
	mh_int_t rc = mh_json_put(tree->hash, key, NULL, NULL);
	if (unlikely(rc == mh_end(tree->hash))) {
		parent->children[insert_idx] = NULL;
		return -1;
	}
	assert(json_tree_lookup(tree, parent, token) == token);
	return 0;
}

void
json_tree_del(struct json_tree *tree, struct json_token *token)
{
	struct json_token *parent = token->parent;
	assert(json_tree_lookup(tree, parent, token) == token);
	struct json_token **child_slot = NULL;
	if (token->key.type == JSON_TOKEN_NUM) {
		child_slot = &parent->children[token->key.num - 1];
	} else {
		uint32_t idx = 0;
		while (idx < parent->child_size &&
		       parent->children[idx] != token) { idx++; }
		if (idx < parent->child_size &&
		       parent->children[idx] == token)
			child_slot = &parent->children[idx];
	}
	assert(child_slot != NULL && *child_slot == token);
	*child_slot = NULL;

	mh_int_t id = mh_json_find(tree->hash, &token, NULL);
	assert(id != mh_end(tree->hash));
	mh_json_del(tree->hash, id, NULL);
	json_token_destroy(token);
	assert(json_tree_lookup(tree, parent, token) == NULL);
}

struct json_token *
json_tree_lookup_path(struct json_tree *tree, struct json_token *parent,
		      const char *path, uint32_t path_len)
{
	int rc;
	struct json_lexer lexer;
	struct json_token token;
	struct json_token *ret = parent != NULL ? parent : &tree->root;
	json_lexer_create(&lexer, path, path_len);
	while ((rc = json_lexer_next_token(&lexer, &token)) == 0 &&
	       token.key.type != JSON_TOKEN_END && ret != NULL) {
		ret = json_tree_lookup(tree, ret, &token);
	}
	if (rc != 0 || token.key.type != JSON_TOKEN_END)
		return NULL;
	return ret;
}

static struct json_token *
json_tree_child_next(struct json_token *parent, struct json_token *pos)
{
	assert(pos == NULL || pos->parent == parent);
	struct json_token **arr = parent->children;
	uint32_t arr_size = parent->child_size;
	if (arr == NULL)
		return NULL;
	uint32_t idx = pos != NULL ? pos->sibling_idx + 1 : 0;
	while (idx < arr_size && arr[idx] == NULL)
		idx++;
	if (idx >= arr_size)
		return NULL;
	return arr[idx];
}

static struct json_token *
json_tree_leftmost(struct json_token *pos)
{
	struct json_token *last;
	do {
		last = pos;
		pos = json_tree_child_next(pos, NULL);
	} while (pos != NULL);
	return last;
}

struct json_token *
json_tree_preorder_next(struct json_token *root, struct json_token *pos)
{
	if (pos == NULL)
		pos = root;
	struct json_token *next = json_tree_child_next(pos, NULL);
	if (next != NULL)
		return next;
	while (pos != root) {
		next = json_tree_child_next(pos->parent, pos);
		if (next != NULL)
			return next;
		pos = pos->parent;
	}
	return NULL;
}

struct json_token *
json_tree_postorder_next(struct json_token *root, struct json_token *pos)
{
	struct json_token *next;
	if (pos == NULL) {
		next = json_tree_leftmost(root);
		return next != root ? next : NULL;
	}
	if (pos == root)
		return NULL;
	next = json_tree_child_next(pos->parent, pos);
	if (next != NULL) {
		next = json_tree_leftmost(next);
		return next != root ? next : NULL;
	}
	return pos->parent != root ? pos->parent : NULL;
}

int
json_path_cmp(const char *a, uint32_t a_len, const char *b, uint32_t b_len)
{
	struct json_lexer lexer_a, lexer_b;
	json_lexer_create(&lexer_a, a, a_len);
	json_lexer_create(&lexer_b, b, b_len);
	struct json_token token_a, token_b;
	int rc_a, rc_b;
	while ((rc_a = json_lexer_next_token(&lexer_a, &token_a)) == 0 &&
		(rc_b = json_lexer_next_token(&lexer_b, &token_b)) == 0 &&
		token_a.key.type != JSON_TOKEN_END &&
		token_b.key.type != JSON_TOKEN_END) {
		int rc = json_token_key_cmp(&token_a, &token_b);
		if (rc != 0)
			return rc;
	}
	/* Path "a" should be valid. */
	assert(rc_a == 0);
	if (rc_b != 0)
		return rc_b;
	/*
	 * The parser stopped because the end of one of the paths
	 * was reached. As JSON_TOKEN_END > JSON_TOKEN_{NUM, STR},
	 * the path having more tokens has lower key.type value.
	 */
	return token_b.key.type - token_a.key.type;
}
