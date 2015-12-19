#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "dbstore.h"

int dbstore(uint8_t which, uint32_t val)
{
	time_t t;
	int i;
	struct tm tm;
	char buf[64];
	char tstr[64];
	char *table = (which == 1) ? "coldcnt" : "hotcnt";

	t = time(NULL);
	(void)gmtime_r(&t, &tm);
	(void)strftime(tstr, sizeof(tstr), "%Y-%m-%d %H:%M:%S", &tm);
	printf("insert into %s values (\"%s\",%u);\n", table, tstr, val);
	return 0;
}
