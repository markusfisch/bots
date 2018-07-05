#include <stdlib.h>
#include <stdint.h>

#include "base64.h"

static const char encoding_table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};
static const unsigned int mod_table[] = { 0, 2, 1 };

char *base64_encode(const unsigned char *data, unsigned int len) {
	unsigned int size = 4 * ((len + 2) / 3) + 1;
	char *encoded = (char *) calloc(size, sizeof(char));
	if (!encoded) {
		return NULL;
	}

	char *enc = encoded;
	const unsigned char *e = data + len;
	while (data < e) {
		uint32_t a = data < e ? *data++ : 0;
		uint32_t b = data < e ? *data++ : 0;
		uint32_t c = data < e ? *data++ : 0;
		uint32_t triple = (a << 0x10) + (b << 0x08) + c;
		*enc++ = encoding_table[(triple >> 3 * 6) & 0x3f];
		*enc++ = encoding_table[(triple >> 2 * 6) & 0x3f];
		*enc++ = encoding_table[(triple >> 1 * 6) & 0x3f];
		*enc++ = encoding_table[(triple >> 0 * 6) & 0x3f];
	}

	unsigned int i = 0;
	for (i = 0; i < mod_table[len % 3]; ++i) {
		encoded[size - 2 - i] = '=';
	}
	encoded[size - 1] = 0;

	return encoded;
}
