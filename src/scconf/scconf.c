/*
 * $Id$
 *
 * Copyright (C) 2002
 *  Antti Tapaninen <aet@cc.hut.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scconf.h"

scconf_context *scconf_new(const char *filename)
{
	scconf_context *config;

	config = (scconf_context *) malloc(sizeof(scconf_context));
	if (!config) {
		return NULL;
	}
	memset(config, 0, sizeof(scconf_context));
	config->filename = filename ? strdup(filename) : NULL;
	config->root = (scconf_block *) malloc(sizeof(scconf_block));
	if (!config->root) {
		if (config->filename) {
			free(config->filename);
		}
		free(config);
		return NULL;
	}
	memset(config->root, 0, sizeof(scconf_block));
	return config;
}

void scconf_free(scconf_context * config)
{
	if (config) {
		scconf_block_destroy(config->root);
		if (config->filename) {
			free(config->filename);
		}
		free(config);
	}
	config = NULL;
}

const scconf_block *scconf_find_block(scconf_context * config, const scconf_block * block, const char *item_name)
{
	scconf_item *item;

	if (!block) {
		block = config->root;
	}
	if (!item_name) {
		return NULL;
	}
	for (item = block->items; item; item = item->next) {
		if (item->type == SCCONF_ITEM_TYPE_BLOCK &&
		    strcasecmp(item_name, item->key) == 0) {
			return item->value.block;
		}
	}
	return NULL;
}

scconf_block **scconf_find_blocks(scconf_context * config, const scconf_block * block, const char *item_name, const char *key)
{
	scconf_block **blocks = NULL;
	int alloc_size, size;
	scconf_item *item;

	if (!block) {
		block = config->root;
	}
	if (!item_name) {
		return NULL;
	}
	size = 0;
	alloc_size = 10;
	blocks = (scconf_block **) realloc(blocks, sizeof(scconf_block *) * alloc_size);

	for (item = block->items; item; item = item->next) {
		if (item->type == SCCONF_ITEM_TYPE_BLOCK &&
		    strcasecmp(item_name, item->key) == 0) {
			if (key && strcasecmp(key, item->value.block->name->data)) {
				continue;
			}
			if (size + 1 >= alloc_size) {
				alloc_size *= 2;
				blocks = (scconf_block **) realloc(blocks, sizeof(scconf_block *) * alloc_size);
			}
			blocks[size++] = item->value.block;
		}
	}
	blocks[size] = NULL;
	return blocks;
}

const scconf_list *scconf_find_list(const scconf_block * block, const char *option)
{
	scconf_item *item;

	if (!block) {
		return NULL;
	}
	for (item = block->items; item; item = item->next) {
		if (item->type == SCCONF_ITEM_TYPE_VALUE &&
		    strcasecmp(option, item->key) == 0) {
			return item->value.list;
		}
	}
	return NULL;
}

const char *scconf_get_str(const scconf_block * block, const char *option, const char *def)
{
	const scconf_list *list;

	list = scconf_find_list(block, option);
	return !list ? def : list->data;
}

int scconf_get_int(const scconf_block * block, const char *option, int def)
{
	const scconf_list *list;

	list = scconf_find_list(block, option);
	return !list ? def : atoi(list->data);
}

int scconf_get_bool(const scconf_block * block, const char *option, int def)
{
	const scconf_list *list;

	list = scconf_find_list(block, option);
	if (!list) {
		return def;
	}
	return toupper((int) *list->data) == 'T' || toupper((int) *list->data) == 'Y';
}

void scconf_block_destroy(scconf_block * block);

static void scconf_items_destroy(scconf_item * items)
{
	scconf_item *next;

	while (items) {
		next = items->next;

		switch (items->type) {
		case SCCONF_ITEM_TYPE_COMMENT:
			if (items->value.comment) {
				free(items->value.comment);
			}
			items->value.comment = NULL;
			break;
		case SCCONF_ITEM_TYPE_BLOCK:
			scconf_block_destroy(items->value.block);
			break;
		case SCCONF_ITEM_TYPE_VALUE:
			scconf_list_destroy(items->value.list);
			break;
		}

		if (items->key) {
			free(items->key);
		}
		items->key = NULL;
		free(items);
		items = next;
	}
}

void scconf_block_destroy(scconf_block * block)
{
	if (block) {
		scconf_list_destroy(block->name);
		scconf_items_destroy(block->items);
		free(block);
	}
}

scconf_list *scconf_list_add(scconf_list ** list, const char *value)
{
	scconf_list *rec, **tmp;

	rec = (scconf_list *) malloc(sizeof(scconf_list));
	if (!rec) {
		return NULL;
	}
	memset(rec, 0, sizeof(scconf_list));
	rec->data = value ? strdup(value) : NULL;

	if (!*list) {
		*list = rec;
	} else {
		for (tmp = list; *tmp; tmp = &(*tmp)->next);
		*tmp = rec;
	}
	return rec;
}

void scconf_list_destroy(scconf_list * list)
{
	scconf_list *next;

	while (list) {
		next = list->next;
		if (list->data) {
			free(list->data);
		}
		free(list);
		list = next;
	}
}

int scconf_list_array_length(const scconf_list * list)
{
	int len = 0;

	while (list) {
		len++;
		list = list->next;
	}
	return len;
}

int scconf_list_strings_length(const scconf_list * list)
{
	int len = 0;

	while (list && list->data) {
		len += strlen(list->data) + 1;
		list = list->next;
	}
	return len;
}

char *scconf_list_strdup(const scconf_list * list, const char *filler)
{
	char *buf = NULL;
	int len = 0;

	if (!list) {
		return NULL;
	}
	len = scconf_list_strings_length(list);
	if (filler) {
		len += scconf_list_array_length(list) * (strlen(filler) + 1);
	}
	buf = (char *) malloc(len);
	if (!buf) {
		return NULL;
	}
	memset(buf, 0, len);
	while (list && list->data) {
		strcat(buf, list->data);
		if (filler) {
			strcat(buf, filler);
		}
		list = list->next;
	}
	if (filler)
		buf[strlen(buf) - strlen(filler)] = '\0';
	return buf;
}

static int parse_entries(scconf_context * config, const scconf_block * block, scconf_entry * entry, int depth);

static int parse_type(scconf_context * config, const scconf_block * block, scconf_entry * entry, int depth)
{
	void *parm = entry->parm;
	int (*callback_func) (scconf_context * config, const scconf_block * block, scconf_entry * entry, int depth) =
	(int (*)(scconf_context *, const scconf_block *, scconf_entry *, int)) parm;
	size_t *len = (size_t *) entry->arg;
	int r = 0;

	if (config->debug) {
		fprintf(stderr, "decoding '%s'\n", entry->name);
	}
	switch (entry->type) {
	case SCCONF_CALLBACK:
		if (parm) {
			r = callback_func(config, block, entry, depth);
		}
		break;
	case SCCONF_BLOCK:
		if (parm) {
			r = parse_entries(config, block, (scconf_entry *) parm, depth + 1);
		}
		break;
	case SCCONF_LIST:
		{
			const scconf_list *val = scconf_find_list(block, entry->name);

			if (!val) {
				r = 1;
				break;
			}
			if (parm) {
				if (entry->flags & SCCONF_ALLOC) {
					scconf_list *dest = NULL;

					for (; val != NULL; val = val->next) {
						if (!scconf_list_add(&dest, val->data)) {
							r = 1;
							break;
						}
					}
					*((scconf_list **) parm) = dest;
				} else {
					*((const scconf_list **) parm) = val;
				}
			}
			if (entry->flags & SCCONF_VERBOSE) {
				char *buf = scconf_list_strdup(val, ", ");
				printf("%s = %s\n", entry->name, buf);
				free(buf);
			}
		}
		break;
	case SCCONF_BOOLEAN:
		{
			int val = scconf_get_bool(block, entry->name, 0);

			if (parm) {
				*((int *) parm) = val;
			}
			if (entry->flags & SCCONF_VERBOSE) {
				printf("%s = %s\n", entry->name, val == 0 ? "false" : "true");
			}
		}
		break;
	case SCCONF_INTEGER:
		{
			int val = scconf_get_int(block, entry->name, 42);

			if (parm) {
				*((int *) parm) = val;
			}
			if (entry->flags & SCCONF_VERBOSE) {
				printf("%s = %i\n", entry->name, val);
			}
		}
		break;
	case SCCONF_STRING:
		{
			const char *val = scconf_get_str(block, entry->name, NULL);
			int vallen = val ? strlen(val) : 0;

			if (!vallen) {
				r = 1;
				break;
			}
			if (parm) {
				if (entry->flags & SCCONF_ALLOC) {
					char **buf = (char **) parm;
					*buf = (char *) malloc(vallen + 1);
					if (*buf == NULL) {
						r = 1;
						break;
					}
					memset(*buf, 0, vallen + 1);
					if (len) {
						*len = vallen;
					}
					parm = *buf;
				}
				memcpy((char *) parm, val, vallen);
			}
			if (entry->flags & SCCONF_VERBOSE) {
				printf("%s = %s\n", entry->name, val);
			}
		}
		break;
	default:
		fprintf(stderr, "invalid configuration type: %d\n", entry->type);
	}
	if (r) {
		fprintf(stderr, "decoding of configuration entry '%s' failed.\n", entry->name);
		return r;
	}
	entry->flags |= SCCONF_PRESENT;
	return 0;
}

static scconf_block **getblocks(scconf_context * config, const scconf_block * block, scconf_entry * entry)
{
	scconf_block **blocks = NULL;

	blocks = scconf_find_blocks(config, block, entry->name, NULL);
	if (blocks) {
		if (blocks[0] != NULL) {
			if (config->debug) {
				fprintf(stderr, "block found (%s)\n", entry->name);
			}
			return blocks;
		}
		free(blocks);
		blocks = NULL;
	}
	if (scconf_find_list(block, entry->name) != NULL) {
		if (config->debug) {
			fprintf(stderr, "list found (%s)\n", entry->name);
		}
		blocks = (scconf_block **) realloc(blocks, sizeof(scconf_block *) * 2);
		if (!blocks)
			return NULL;
		blocks[0] = (scconf_block *) block;
		blocks[1] = NULL;
	}
	return blocks;
}

static int parse_entries(scconf_context * config, const scconf_block * block, scconf_entry * entry, int depth)
{
	int r, i, idx;
	scconf_entry *e;
	scconf_block **blocks = NULL;

	if (config->debug) {
		fprintf(stderr, "parse_entries called, depth %d\n", depth);
	}
	for (idx = 0; entry[idx].name; idx++) {
		e = &entry[idx];
		r = 0;
		blocks = getblocks(config, block, e);
		if (!blocks) {
			if (!(e->flags & SCCONF_MANDATORY)) {
				if (config->debug)
					fprintf(stderr, "optional configuration entry '%s' not present\n",
						e->name);
				continue;
			}
			fprintf(stderr, "mandatory configuration entry '%s' not found\n", e->name);
			return 1;
		}
		for (i = 0; blocks[i]; i++) {
			r = parse_type(config, blocks[i], e, depth);
			if (r) {
				free(blocks);
				return r;
			}
			if (!(e->flags & SCCONF_ALL_BLOCKS))
				break;
		}
		free(blocks);
	}
	return 0;
}

int scconf_parse_entries(scconf_context * config, const scconf_block * block, scconf_entry * entry)
{
	if (!entry)
		return 1;
	if (!block)
		block = config->root;
	return parse_entries(config, block, entry, 0);
}
