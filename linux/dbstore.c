#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include <glib.h>

#include <mysql/mysql.h>

#include "dbstore.h"

static char *host = NULL;
static char *user = NULL;
static char *pass = NULL;
static char *dbnm = "watermeter";

int dbconfig(char *conffile)
{
	FILE *fp = fopen(conffile, "r");
	int rc = 0;
	char buf[128];

	if (!fp)
		return 1;
	while (fgets(buf, sizeof(buf), fp)) {
		char *k, *v, *e;

		e = buf + strlen(buf) - 1;
		if (*e == '\n')
			*e = '\0';
		else {
			/* line too long */
			rc = 1;
			break;
		}
		for (k = buf; k < e && isspace(k); k++) /*nothing*/ ;
		if (*k == '#') break;
		for (v = k; v < e && !isspace(v)
			    && *v != ':' && *v != '='; v++) /*nothing*/ ;
		if (v < e && (*v == ':' || *v == '=')) v++;
		for (; v < e && (isspace(v) || *v == ':' || *v == '=')
		     					; v++) /*nothing*/ ;
		if (v >= e) {
			/* no value */
			rc = 1;
			break;
		}
		if      (!strcmp(k, "host"))     host = strdup(v);
		else if (!strcmp(k, "user"))     user = strdup(v);
		else if (!strcmp(k, "password")) pass = strdup(v);
		else if (!strcmp(k, "database")) dbnm = strdup(v);
		else {
			/* unknown key */
			rc = 1;
			break;
		}
	}
	return rc;
}

int dbstore(uint8_t which, uint32_t val)
{
	time_t t;
	int i;
	MYSQL mysql;
	int rc = 0;
	struct tm tm;
	char buf[64];
	char tstr[32];
	char *table = (which == 1) ? "cold" : "hot";
	char statement[64];
	MYSQL_RES *result;
	uint32_t prev_val = 0;

	t = time(NULL);
	(void)gmtime_r(&t, &tm);
	(void)strftime(tstr, sizeof(tstr), "%Y-%m-%d %H:%M:%S", &tm);
	mysql_init(&mysql);
	if(!mysql_real_connect(&mysql, host, user, pass, dbnm, 0, NULL, 0)) {
		g_warning("mysql connect error: %s\n", mysql_error(&mysql));
		return 1;
	}
	mysql_autocommit(&mysql, FALSE);
	/* ======== */
	snprintf(statement, sizeof(statement),
		 "select value from %scnt order by timestamp desc limit 1;\n",
		 table);
	if ((rc = mysql_query(&mysql, statement)))
		g_warning("mysql \"%s\" error: %s\n",
				statement, mysql_error(&mysql));
	else if ((result = mysql_store_result(&mysql))){
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row && *row) prev_val = atoi(*row);
		mysql_free_result(result);
	}
	if (val <= prev_val) {
		snprintf(statement, sizeof(statement),
			 "insert into %sadj values (\"%s\",%u);\n",
			 table, tstr, prev_val);
		g_info("%s %u <= %u, %s", table, val, prev_val, statement);
		if ((rc = mysql_query(&mysql, statement)))
			g_warning("mysql \"%s\" error: %s\n",
					statement, mysql_error(&mysql));
	}
	snprintf(statement, sizeof(statement),
		 "insert into %scnt values (\"%s\",%u);\n",
		 table, tstr, val);
	if ((rc = mysql_query(&mysql, statement)))
		g_warning("mysql \"%s\" error: %s\n",
				statement, mysql_error(&mysql));
	/* ======== */
	if (!rc) {
		if ((rc = mysql_commit(&mysql)))
			g_warning("mysql commit error: %s\n",
					mysql_error(&mysql));
	}
	mysql_close(&mysql);
	return rc;
}
