#include "bind.h"
#include "key.h"
#include "common.h"
#include "error.h"
#include "command.h"
#include "ptr-array.h"

struct key_chain {
	int keys[3];
	int count;
};

struct binding {
	char *command;
	struct key_chain chain;
};

static struct key_chain pressed_keys;

static PTR_ARRAY(bindings);

static bool parse_keys(struct key_chain *chain, const char *str)
{
	char *keys = xstrdup(str);
	int len = strlen(keys);
	int i = 0;

	// convert all whitespace to \0
	for (i = 0; i < len; i++) {
		if (isspace(keys[i])) {
			keys[i] = 0;
		}
	}

	clear(chain);
	for (i = 0; i < len; ) {
		const char *key;

		while (i < len && keys[i] == 0) {
			i++;
		}
		key = keys + i;
		while (i < len && keys[i] != 0) {
			i++;
		}
		if (key == keys + i)
			break;

		if (chain->count >= ARRAY_COUNT(chain->keys)) {
			error_msg("Too many keys.");
			free(keys);
			return false;
		}
		if (!parse_key(&chain->keys[chain->count], key)) {
			error_msg("Invalid key %s", key);
			free(keys);
			return false;
		}
		chain->count++;
	}
	free(keys);
	if (chain->count == 0) {
		error_msg("Empty key not allowed.");
		return false;
	}
	return true;
}

void add_binding(const char *keys, const char *command)
{
	struct binding *b;

	b = xnew(struct binding, 1);
	if (!parse_keys(&b->chain, keys)) {
		free(b);
		return;
	}

	b->command = xstrdup(command);
	ptr_array_add(&bindings, b);
}

void remove_binding(const char *keys)
{
	struct key_chain chain;
	int i = bindings.count;

	if (!parse_keys(&chain, keys))
		return;

	while (i > 0) {
		struct binding *b = bindings.ptrs[--i];

		if (memcmp(&b->chain, &chain, sizeof(chain)) == 0) {
			ptr_array_remove_idx(&bindings, i);
			free(b->command);
			free(b);
			return;
		}
	}
}

void handle_binding(int key)
{
	int i;

	pressed_keys.keys[pressed_keys.count] = key;
	pressed_keys.count++;

	for (i = bindings.count; i > 0; i--) {
		struct binding *b = bindings.ptrs[i - 1];
		struct key_chain *c = &b->chain;

		if (c->count < pressed_keys.count)
			continue;

		if (memcmp(c->keys, pressed_keys.keys, pressed_keys.count * sizeof(pressed_keys.keys[0])))
			continue;

		if (c->count > pressed_keys.count)
			return;

		handle_command(commands, b->command);
		break;
	}
	clear(&pressed_keys);
}

int nr_pressed_keys(void)
{
	return pressed_keys.count;
}
