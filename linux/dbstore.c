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
	int lc = 0;
	int rc = 0;
	char buf[128];

	if (!fp) return 1;
	while (fgets(buf, sizeof(buf), fp)) {
		char *k, *v, *e;

		lc++;
		e = buf + strlen(buf) - 1;
		if (*e == '\n') *e = '\0';
		else {
			g_warning("%s:%d line too long", conffile, lc);
			rc = 1;
			break;
		}
		if ((k = strchr(buf, '#'))) {
			e=k;
			*e = '\0';
		}
		for (k = buf; k < e && isspace(*k); k++) /*nothing*/ ;
		for (v = k; v < e && !isspace(*v)
			    && *v != ':' && *v != '='; v++) /*nothing*/ ;
		*v++ = '\0';
		if (*k == '\0') continue; /* empty or comment-only line */
		for (; v < e && (isspace(*v) || *v == ':' || *v == '=')
		     					; v++) /*nothing*/ ;
		if (v >= e) {
			g_warning("%s:%d no value for key \"%s\"",
					conffile, lc, k);
			rc = 1;
			break;
		} 
#ifdef TEST_CONFIG
		printf("k=%s v=%s\n", k, v);
#endif
		if      (!strcmp(k, "host"))     host = strdup(v);
		else if (!strcmp(k, "user"))     user = strdup(v);
		else if (!strcmp(k, "password")) pass = strdup(v);
		else if (!strcmp(k, "database")) dbnm = strdup(v);
		else {
			g_warning("%s:%d unrecognized key \"%s\"",
					conffile, lc, k);
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
	char tstr[32], prevtstr[32];
	char *table = (which == 1) ? "cold" : "hot";
	char statement[256];
	MYSQL_RES *result;
	uint32_t prev_val = 0;
	int bogus = 0;

	t = time(NULL);
	(void)gmtime_r(&t, &tm);
	(void)strftime(tstr, sizeof(tstr), "%Y-%m-%d %H:%M:%S", &tm);
	mysql_init(&mysql);
	if(!mysql_real_connect(&mysql, host, user, pass, dbnm, 0, NULL, 0)) {
		g_warning("mysql connect error: %s\n", mysql_error(&mysql));
		return 1;
	}
	mysql_autocommit(&mysql, FALSE);
	/* ==== Was the sensor reset since last measurement? ==== */
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
	if (val < prev_val) {
		snprintf(statement, sizeof(statement),
			 "insert into %sadj values (\"%s\",%u);\n",
			 table, tstr, prev_val);
		g_info("%s %u <= %u, %s", table, val, prev_val, statement);
		if ((rc = mysql_query(&mysql, statement)))
			g_warning("mysql \"%s\" error: %s\n",
					statement, mysql_error(&mysql));
	}
	/* ==== Is this a part of a series of bogus events? ==== */
	snprintf(statement, sizeof(statement),
		 "select timestamp from %scnt "
		 "where timestamp > subtime(\"%s\", \"0 0:0:10\") "
		 "order by timestamp desc;\n",
		 table, tstr);
	if ((rc = mysql_query(&mysql, statement)))
		g_warning("mysql \"%s\" error: %s\n",
				statement, mysql_error(&mysql));
	else if ((result = mysql_store_result(&mysql))){
		my_ulonglong rows = mysql_num_rows(result);

		if (rows > 0) {
			bogus = 1;
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row && *row) {
				strncpy(prevtstr, *row, sizeof(prevtstr));
			} else {
				g_warning("mysql_fetch_row after \"%s\": "
					  "no result despite rows=%llu\n",
					  statement, rows);
				bogus = -1;
			}
		}
		if (rows == 1)
			bogus = 2;

		mysql_free_result(result);
	}
	if (bogus > 0) {
		snprintf(statement, sizeof(statement),
			 "insert into %sadj values (\"%s\", -1);\n",
			 table, tstr);
		g_info("%s %u at \"%s\" bogus, %s",
			table, val, tstr, statement);
		if ((rc = mysql_query(&mysql, statement)))
			g_warning("mysql \"%s\" error: %s\n",
					statement, mysql_error(&mysql));
	}
	if (bogus > 1) {
		snprintf(statement, sizeof(statement),
			 "insert into %sadj values (\"%s\", -1);\n",
			 table, prevtstr);
		g_info("previous %s at \"%s\" was bogus too, %s",
			table, prevtstr, statement);
		if ((rc = mysql_query(&mysql, statement)))
			g_warning("mysql \"%s\" error: %s\n",
					statement, mysql_error(&mysql));
	}
	/* ==== Update the counter table regardless ==== */
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

#ifdef TEST_CONFIG
int main(int const argc, char *argv[])
{
	if (dbconfig(argv[1])) {
		printf("could not parse config file\n");
		return 1;
	}
	printf("host: %s\nuser: %s\npass: %s\ndbnm: %s\n",
		host, user, pass, dbnm);
	return 0;
}
#endif
