#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

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
	{ NULL },
};

static gboolean channel_watcher(GIOChannel *chan, GIOCondition cond,
				gpointer user_data)
{
	g_io_channel_shutdown(chan, FALSE, NULL);
	g_io_channel_unref(chan);
	g_main_loop_quit(event_loop);
	g_print("channel_watcher cleared channel and exiting\n");
	return FALSE;
}

static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
	GAttrib *attrib = user_data;
	uint8_t *opdu;
	uint16_t handle, i, olen = 0;
	size_t plen;
	time_t t;
	struct tm tm;
	char tstr[64];

	t = time(NULL);
	(void)gmtime_r(&t, &tm);
	(void)strftime(tstr, sizeof(tstr), "%Y-%m-%d %H:%M:%S", &tm);
	handle = bt_get_le16(&pdu[1]);
	g_print("%s ev %02x hd 0x%04x value: ", tstr, pdu[0], handle);
	for (i = 3; i < len; i++)
		g_print("%02x ", pdu[i]);
	g_print("\n");
}

static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
	GAttrib *attrib;
	uint16_t mtu;
	uint16_t cid;
	GError *gerr = NULL;

	if (err) {
		g_printerr("%s\n", err->message);
		g_main_loop_quit(event_loop);
	}
	bt_io_get(io, &gerr, BT_IO_OPT_IMTU, &mtu,
				BT_IO_OPT_CID, &cid, BT_IO_OPT_INVALID);
	if (gerr) {
		g_printerr("Can't detect MTU, using default: %s",
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
	g_print("connect_cb registered events_handler and exiting\n");
}

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	GIOChannel *chan;
	gboolean got_error = FALSE;

	opt_dst_type = g_strdup("public");
	opt_sec_level = g_strdup("low");
	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
		g_printerr("%s\n", gerr->message);
		g_clear_error(&gerr);
		got_error = TRUE;
		goto done;
	}
	chan = gatt_connect(opt_src, opt_dst, opt_dst_type, opt_sec_level,
				opt_psm, opt_mtu, connect_cb, &gerr);
	if (chan == NULL) {
		g_printerr("%s\n", gerr->message);
		g_clear_error(&gerr);
		got_error = TRUE;
		goto done;
	} else {
		g_io_add_watch(chan, G_IO_HUP, channel_watcher, NULL);
	}

	event_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(event_loop);
	g_main_loop_unref(event_loop);

done:
	g_option_context_free(context);
	g_free(opt_src);
	g_free(opt_dst);
	g_free(opt_sec_level);
	if (got_error)
		exit(EXIT_FAILURE);
	else
		exit(EXIT_SUCCESS);
}
