#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <syslog.h>

#include <glib.h>

#include <lib/bluetooth.h>
#include <lib/hci.h>
#include <lib/hci_lib.h>
#include <lib/sdp.h>
#include "lib/uuid.h"

#include "src/shared/util.h"
#include "attrib/att.h"
#include "btio/btio.h"
#include "attrib/gattrib.h"
#include "attrib/gatt.h"

#include "dbstore.h"

GIOChannel *gatt_connect(const char *src, const char *dst,
			const char *dst_type, const char *sec_level,
			int psm, int mtu, BtIOConnect connect_cb,
			GError **gerr);

static char *opt_src = NULL;
static char *opt_dst = NULL;
static char *opt_dst_type = NULL;
static int opt_mtu = 0;
static int opt_psm = 0;
static char *opt_sec_level = NULL;
static char *opt_dbconffile = NULL;
static gboolean opt_daemon = FALSE;

static GMainLoop *event_loop;

static GOptionEntry options[] = {
	{ "adapter", 'i', 0, G_OPTION_ARG_STRING, &opt_src,
		"Specify local adapter interface", "hciX" },
	{ "device", 'b', 0, G_OPTION_ARG_STRING, &opt_dst,
		"Specify remote Bluetooth address", "MAC" },
	{ "addr-type", 't', 0, G_OPTION_ARG_STRING, &opt_dst_type,
		"Set LE address type. Default: public", "[public | random]"},
	{ "mtu", 'm', 0, G_OPTION_ARG_INT, &opt_mtu,
		"Specify the MTU size", "MTU" },
	{ "psm", 'p', 0, G_OPTION_ARG_INT, &opt_psm,
		"Specify the PSM for GATT/ATT over BR/EDR", "PSM" },
	{ "sec-level", 'l', 0, G_OPTION_ARG_STRING, &opt_sec_level,
		"Set security level. Default: low", "[low | medium | high]"},
	{ "dbconfig", 'c', 0, G_OPTION_ARG_FILENAME, &opt_dbconffile,
		"Specify file name with database configuration", "cfile"},
	{ "daemon", 'd', 0, G_OPTION_ARG_NONE, &opt_daemon,
		"Specify file name with database configuration", "cfile"},
	{ NULL },
};

void local_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
			const gchar *message, gpointer log_context)
{
	int syslog_level;

	switch (log_level) {
	case G_LOG_LEVEL_CRITICAL:	syslog_level = LOG_CRIT;	break;
	case G_LOG_LEVEL_ERROR:		syslog_level = LOG_ERR;		break;
	case G_LOG_LEVEL_WARNING:	syslog_level = LOG_WARNING;	break;
	case G_LOG_LEVEL_MESSAGE:	syslog_level = LOG_NOTICE;	break;
	case G_LOG_LEVEL_INFO:		syslog_level = LOG_INFO;	break;
	case G_LOG_LEVEL_DEBUG:		syslog_level = LOG_DEBUG;	break;
	default:			syslog_level = LOG_INFO;
	}
	if (!log_domain || (log_domain[0] == '\0'))
		syslog(syslog_level, "%s", message);
	else
		syslog(syslog_level, "%s: %s", log_domain, message);
}

static gboolean channel_watcher(GIOChannel *chan, GIOCondition cond,
				gpointer user_data)
{
	g_io_channel_shutdown(chan, FALSE, NULL);
	g_io_channel_unref(chan);
	g_main_loop_quit(event_loop);
	g_info("channel_watcher cleared channel and exiting");
	return FALSE;
}

static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
	GAttrib *attrib = user_data;
	uint8_t *opdu;
	uint8_t which;
	uint16_t handle;

	handle = bt_get_le16(&pdu[1]);
	which = pdu[3];
	if ((pdu[0] == 0x1b) && (handle == 0x0012) && (len == 9)) {
		uint32_t val = bt_get_le32(&pdu[5]);

		if ((which == 1) || (which == 2)) {
			g_debug("store: \"%hhu,%u\"\n", which, val);
			if (dbstore(which, val))
				g_warning("error storing \"%hhu,%u\"\n",
						which, val);
		} else {
			g_debug("jitter: \"%hhu,%u\"\n", which, val);
		}
	} else {
		time_t t;
		int i;
		struct tm tm;
		char buf[64];
		char tstr[64];

		t = time(NULL);
		(void)gmtime_r(&t, &tm);
		(void)strftime(tstr, sizeof(tstr), "%Y-%m-%d %H:%M:%S", &tm);
		for (i = 3; (i < len) && ((i-3) < (sizeof(buf)/3)); i++)
			sprintf(buf+strlen(buf), " %02x ", pdu[i]);
		g_warning("%s ev %02x hd 0x%04x value: %s",
			tstr, pdu[0], handle, buf);
	}
}

static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
	GAttrib *attrib;
	uint16_t mtu;
	uint16_t cid;
	GError *gerr = NULL;

	if (err) {
		g_warning("%s", err->message);
		g_main_loop_quit(event_loop);
	}
	bt_io_get(io, &gerr, BT_IO_OPT_IMTU, &mtu,
				BT_IO_OPT_CID, &cid, BT_IO_OPT_INVALID);
	if (gerr) {
		g_warning("Can't detect MTU, using default: %s",
							gerr->message);
		g_error_free(gerr);
		mtu = ATT_DEFAULT_LE_MTU;
	}
	if (cid == ATT_CID)
		mtu = ATT_DEFAULT_LE_MTU;
	attrib = g_attrib_new(io, mtu, false);
	g_attrib_register(attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
						events_handler, attrib, NULL);
	g_attrib_register(attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
						events_handler, attrib, NULL);
	g_info("connect_cb registered events_handler and exiting\n");
}

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	GIOChannel *chan;
	gboolean got_error = FALSE;

	g_log_set_handler(NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
			  | G_LOG_FLAG_RECURSION, local_log_handler, NULL);
	opt_dst_type = g_strdup("public");
	opt_sec_level = g_strdup("low");
	opt_dbconffile = g_strdup("/etc/pulsecounter.db");
	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		g_error("%s", gerr->message);
		g_clear_error(&gerr);
		got_error = TRUE;
		goto done;
	}
	if (!opt_dst) {
		g_error("Destination MAC address must be specified");
		got_error = TRUE;
		goto done;
	}
	if (dbconfig(opt_dbconffile)) {
		g_error("Could not parse database configuration file");
		got_error = TRUE;
		goto done;
	}
	if (opt_daemon) daemon(0, 0);
	while (1) {
		chan = gatt_connect(opt_src, opt_dst, opt_dst_type,
			opt_sec_level, opt_psm, opt_mtu, connect_cb, &gerr);
		if (chan) {
			g_io_add_watch(chan, G_IO_HUP, channel_watcher, NULL);
			event_loop = g_main_loop_new(NULL, FALSE);
			g_main_loop_run(event_loop);
			g_main_loop_unref(event_loop);
		} else {
			g_warning("%s", gerr->message);
			g_clear_error(&gerr);
			got_error = TRUE;
		}
		sleep(10);
	}

done:
	g_option_context_free(context);
	g_free(opt_src);
	g_free(opt_dst);
	g_free(opt_sec_level);
	g_free(opt_dbconffile);
	if (got_error)
		exit(EXIT_FAILURE);
	else
		exit(EXIT_SUCCESS);
}
