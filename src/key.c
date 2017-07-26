#include "key.h"
#include "uchar.h"
#include "ctype.h"
#include "gbuf.h"

static const char *special_names[NR_SPECIAL_KEYS] = {
	"insert",
	"delete",
	"home",
	"end",
	"pgup",
	"pgdown",
	"left",
	"right",
	"up",
	"down",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
};

static int parse_modifiers(const char *str, int *modifiersp)
{
	int modifiers = 0;
	int i = 0;

	while (true) {
		unsigned char ch = toupper(str[i]);

		if (ch == '^' && str[i + 1] != 0) {
			modifiers |= MOD_CTRL;
			i++;
		} else if (ch == 'C' && str[i + 1] == '-') {
			modifiers |= MOD_CTRL;
			i += 2;
		} else if (ch == 'M' && str[i + 1] == '-') {
			modifiers |= MOD_META;
			i += 2;
		} else if (ch == 'S' && str[i + 1] == '-') {
			modifiers |= MOD_SHIFT;
			i += 2;
		} else {
			break;
		}
	}
	*modifiersp = modifiers;
	return i;
}

bool parse_key(int *key, const char *str)
{
	int modifiers, ch;
	long i, len;

	str += parse_modifiers(str, &modifiers);
	len = strlen(str);

	i = 0;
	ch = u_get_char(str, len, &i);
	if (u_is_unicode(ch) && i == len) {
		if (modifiers == MOD_CTRL) {
			// normalize
			switch (ch) {
			case 'i':
			case 'I':
				ch = '\t';
				modifiers = 0;
				break;
			case 'm':
			case 'M':
				ch = KEY_ENTER;
				modifiers = 0;
				break;
			}
		}
		*key = modifiers | ch;
		return true;
	}
	if (!strcasecmp(str, "sp") || !strcasecmp(str, "space")) {
		*key = modifiers | ' ';
		return true;
	}
	if (!strcasecmp(str, "tab")) {
		*key = modifiers | '\t';
		return true;
	}
	if (!strcasecmp(str, "enter")) {
		*key = modifiers | KEY_ENTER;
		return true;
	}
	for (i = 0; i < NR_SPECIAL_KEYS; i++) {
		if (!strcasecmp(str, special_names[i])) {
			*key = modifiers | (KEY_SPECIAL_MIN + i);
			return true;
		}
	}
	return false;
}

char *key_to_string(int key)
{
	GBUF(buf);

	if (key & MOD_CTRL) {
		gbuf_add_str(&buf, "C-");
	}
	if (key & MOD_META) {
		gbuf_add_str(&buf, "M-");
	}
	if (key & MOD_SHIFT) {
		gbuf_add_str(&buf, "S-");
	}
	key = key & ~MOD_MASK;
	if (u_is_unicode(key)) {
		switch (key) {
		case '\t':
			gbuf_add_str(&buf, "tab");
			break;
		case KEY_ENTER:
			gbuf_add_str(&buf, "enter");
			break;
		case ' ':
			gbuf_add_str(&buf, "space");
			break;
		default:
			// <0x20 or 0x7f shouldn't be possible
			gbuf_add_ch(&buf, key);
		}
	} else if (key >= KEY_SPECIAL_MIN && key <= KEY_SPECIAL_MAX) {
		gbuf_add_str(&buf, special_names[key - KEY_SPECIAL_MIN]);
	} else if (key == KEY_PASTE) {
		gbuf_add_str(&buf, "paste");
	} else {
		gbuf_add_str(&buf, "???");
	}
	return gbuf_steal_cstring(&buf);
}

bool key_to_ctrl(int key, unsigned char *byte)
{
	if ((key & MOD_MASK) != MOD_CTRL) {
		return false;
	}
	key &= ~MOD_CTRL;
	if (key >= '@' && key <= '_') {
		*byte = key & ~0x40;
		return true;
	}
	if (key == '?') {
		*byte = 0x7f;
		return true;
	}
	return false;
}
