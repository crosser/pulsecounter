#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <mysql/mysql.h>

#include "dbstore.h"

int dbstore(uint8_t which, uint32_t val)
{
	time_t t;
	int i;
	MYSQL mysql;
	int rc = 0;
	struct tm tm;
	char buf[64];
	char tstr[32];
	char *table = (which == 1) ? "coldcnt" : "hotcnt";
	char statement[64];

	t = time(NULL);
	(void)gmtime_r(&t, &tm);
	(void)strftime(tstr, sizeof(tstr), "%Y-%m-%d %H:%M:%S", &tm);
	mysql_init(&mysql);
	if(!mysql_real_connect(&mysql, NULL, "pulsecounter",
				"xxxxxxxxxxxxx", "watermeter", 0, NULL, 0)) {
		fprintf(stderr, "mysql connect error: %s\n",
			mysql_error(&mysql));
		return 1;
	}
	snprintf(statement, sizeof(statement),
		 "insert into %s values (\"%s\",%u);\n",
		 table, tstr, val);
	rc = mysql_query(&mysql, statement);
	if (rc)
		fprintf(stderr, "mysql insert \"%s\" error: %s\n",
			statement, mysql_error(&mysql));
	mysql_close(&mysql);
	return rc;
}
