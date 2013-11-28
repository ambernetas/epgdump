#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "iso_639_conversion.h"

struct table_s {
	const char *key;
	const char *val;
};

struct table_s iso639_conversion_table[] = {
#include "iso_639.map.h"
};

char buf[4];

const char *iso_639_2to1(const uint8_t *cc)
{
	int i;

	buf[0] = tolower(cc[0]);
	buf[1] = tolower(cc[1]);
	buf[2] = tolower(cc[2]);
	buf[3] = '\0';

	for (i = 0; iso639_conversion_table[i].key; i++) {
		if (strncmp(buf, iso639_conversion_table[i].key, 3) == 0)
			return iso639_conversion_table[i].val;
	}
	return buf;
}

