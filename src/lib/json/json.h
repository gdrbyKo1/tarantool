#ifndef TARANTOOL_JSON_JSON_H_INCLUDED
#define TARANTOOL_JSON_JSON_H_INCLUDED
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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Lexer for JSON paths:
 * <field>, <.field>, <[123]>, <['field']> and their combinations.
 */
struct json_lexer {
	/** Source string. */
	const char *src;
	/** Length of string. */
	int src_len;
	/** Current lexer's offset in bytes. */
	int offset;
	/** Current lexer's offset in symbols. */
	int symbol_count;
};

enum json_token_type {
	JSON_TOKEN_NUM,
	JSON_TOKEN_STR,
	/** Lexer reached end of path. */
	JSON_TOKEN_END,
};

/**
 * Element of a JSON path. It can be either string or number.
 * String idenfiers are in ["..."] and between dots. Numbers are
 * indexes in [...]. May be organized in a JSON tree.
 */
struct json_token {
	struct {
		enum json_token_type type;
		union {
			struct {
				/** String identifier. */
				const char *str;
				/** Length of @a str. */
				int len;
			};
			/** Index value. */
			uint64_t num;
		};
	} key;
	/** Rolling hash for node used to lookup in json_tree. */
	uint32_t rolling_hash;
	/**
	 * Array of child records. Indexes in this array
	 * follows array indexe-1 for JSON_TOKEN_NUM token type
	 * and are allocated sequently for JSON_TOKEN_NUM child
	 * tokens. NULLs initializations are performed with new
	 * entries allocation.
	 */
	struct json_token **children;
	/** Allocation size of children array. */
	uint32_t child_size;
	/**
	 * Count of defined children array items. Equal the
	 * maximum index of inserted item.
	 */
	uint32_t child_count;
	/** Index of node in parent children array. */
	uint32_t sibling_idx;
	/** Pointer to parent node. */
	struct json_token *parent;
};

struct mh_json_t;

/** JSON tree object to manage tokens relations. */
struct json_tree {
	/** JSON tree root node. */
	struct json_token root;
	/** Hashtable af all tree nodes. */
	struct mh_json_t *hash;
};

/**
 * Create @a lexer.
 * @param[out] lexer Lexer to create.
 * @param src Source string.
 * @param src_len Length of @a src.
 */
static inline void
json_lexer_create(struct json_lexer *lexer, const char *src, int src_len)
{
	lexer->src = src;
	lexer->src_len = src_len;
	lexer->offset = 0;
	lexer->symbol_count = 0;
}

/**
 * Get a next path token.
 * @param lexer Lexer.
 * @param[out] token Token to store parsed result.
 * @retval   0 Success. For result see @a token.str, token.len,
 *             token.num.
 * @retval > 0 Position of a syntax error. A position is 1-based
 *             and starts from a beginning of a source string.
 */
int
json_lexer_next_token(struct json_lexer *lexer, struct json_token *token);

/**
 * Compare two JSON paths using Lexer class.
 * - @a path must be valid
 * - at the case of paths that have same token-sequence prefix,
 *   the path having more tokens is assumed to be greater
 * - when @b path contains an error, the path "a" is assumed to
 *   be greater
 */
int
json_path_cmp(const char *a, uint32_t a_len, const char *b, uint32_t b_len);

/** Create a JSON tree object to manage data relations. */
int
json_tree_create(struct json_tree *tree);

/**
 * Destroy JSON tree object. This routine doesn't destroy attached
 * subtree so it should be called at the end of manual destroy.
 */
void
json_tree_destroy(struct json_tree *tree);

/**
 * Make child lookup in JSON tree by token at position specified
 * with parent. The parent may be set NULL to use tree root
 * record.
 */
struct json_token *
json_tree_lookup(struct json_tree *tree, struct json_token *parent,
		 struct json_token *token);

/**
 * Append token to the given parent position in a JSON tree. The
 * parent mustn't have a child with such content. The parent may
 * be set NULL to use tree root record.
 */
int
json_tree_add(struct json_tree *tree, struct json_token *parent,
	      struct json_token *token);

/**
 * Delete a JSON tree token at the given parent position in JSON
 * tree. Token entry shouldn't have subtree.
 */
void
json_tree_del(struct json_tree *tree, struct json_token *token);

/** Make child lookup by path in JSON tree. */
struct json_token *
json_tree_lookup_path(struct json_tree *tree, struct json_token *parent,
		      const char *path, uint32_t path_len);

/** Make pre order traversal in JSON tree. */
struct json_token *
json_tree_preorder_next(struct json_token *root, struct json_token *pos);

/** Make post order traversal in JSON tree. */
struct json_token *
json_tree_postorder_next(struct json_token *root, struct json_token *pos);

/**
 * Make safe post-order traversal in JSON tree.
 * May be used for destructors.
 */
#define json_tree_foreach_safe(node, root)				     \
for (struct json_token *__next = json_tree_postorder_next((root), NULL);     \
     (((node) = __next) &&						     \
     (__next = json_tree_postorder_next((root), (node))), (node) != NULL);)

#ifndef typeof
/* TODO: 'typeof' is a GNU extension */
#define typeof __typeof__
#endif

/** Return container entry by json_tree_node node. */
#define json_tree_entry(node, type, member) ({ 				     \
	const typeof( ((type *)0)->member ) *__mptr = (node);		     \
	(type *)( (char *)__mptr - ((size_t) &((type *)0)->member) );	     \
})

/**
 * Return container entry by json_tree_node or NULL if
 * node is NULL.
 */
#define json_tree_entry_safe(node, type, member) ({			     \
	(node) != NULL ? json_tree_entry((node), type, member) : NULL;	     \
})

/** Make entry pre order traversal in JSON tree  */
#define json_tree_preorder_next_entry(node, root, type, member) ({	     \
	struct json_token *__next =					     \
		json_tree_preorder_next((root), (node));		     \
	json_tree_entry_safe(__next, type, member);			     \
})

/** Make entry post order traversal in JSON tree  */
#define json_tree_postorder_next_entry(node, root, type, member) ({	     \
	struct json_token *__next =					     \
		json_tree_postorder_next((root), (node));		     \
	json_tree_entry_safe(__next, type, member);			     \
})

/** Make lookup in tree by path and return entry. */
#define json_tree_lookup_path_entry(tree, parent, path, path_len, type,	     \
				    member)				     \
({struct json_token *__node =						     \
	json_tree_lookup_path((tree), (parent), path, path_len);	     \
	json_tree_entry_safe(__node, type, member); })

/** Make lookup in tree by token and return entry. */
#define json_tree_lookup_entry(tree, parent, token, type, member)	     \
({struct json_token *__node =						     \
	json_tree_lookup((tree), (parent), token);			     \
	json_tree_entry_safe(__node, type, member);			     \
})

/** Make pre-order traversal in JSON tree. */
#define json_tree_foreach_preorder(node, root)				     \
for ((node) = json_tree_preorder_next((root), NULL); (node) != NULL;	     \
     (node) = json_tree_preorder_next((root), (node)))

/** Make post-order traversal in JSON tree. */
#define json_tree_foreach_postorder(node, root)				     \
for ((node) = json_tree_postorder_next((root), NULL); (node) != NULL;	     \
     (node) = json_tree_postorder_next((root), (node)))

/** Make post-order traversal in JSON tree and return entry. */
#define json_tree_foreach_entry_preorder(node, root, type, member)	     \
for ((node) = json_tree_preorder_next_entry(NULL, (root), type, member);     \
     (node) != NULL;							     \
     (node) = json_tree_preorder_next_entry(&(node)->member, (root), type,   \
					    member))

/** Make pre-order traversal in JSON tree and return entry. */
#define json_tree_foreach_entry_postorder(node, root, type, member)	     \
for ((node) = json_tree_postorder_next_entry(NULL, (root), type, member);    \
     (node) != NULL;							     \
     (node) = json_tree_postorder_next_entry(&(node)->member, (root), type,  \
					     member))

/**
 * Make secure post-order traversal in JSON tree and return entry.
 */
#define json_tree_foreach_entry_safe(node, root, type, member)		     \
for (type *__next = json_tree_postorder_next_entry(NULL, (root), type,	     \
						   member);		     \
     (((node) = __next) &&						     \
     (__next = json_tree_postorder_next_entry(&(node)->member, (root), type, \
					      member)),			     \
     (node) != NULL);)


#ifdef __cplusplus
}
#endif

#endif /* TARANTOOL_JSON_JSON_H_INCLUDED */
