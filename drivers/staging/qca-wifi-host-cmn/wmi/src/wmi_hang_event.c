/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <qdf_hang_event_notifier.h>
#include <qdf_notifier.h>
#include <wmi_hang_event.h>
#include <wmi_unified_priv.h>
#include <qdf_trace.h>

struct wmi_hang_data_fixed_param {
	uint32_t tlv_header; /* tlv tag and length */
	uint32_t event;
	uint32_t data;
	uint64_t time;
} qdf_packed;

#define WMI_EVT_HIST 0
#define WMI_CMD_HIST 1

static void wmi_log_history(struct notifier_block *block, void *data,
			    uint8_t wmi_history)
{
	qdf_notif_block *notif_block = qdf_container_of(block, qdf_notif_block,
							notif_block);
	struct qdf_notifer_data *wmi_hang_data = data;
	int nread, pos, total_len;
	unsigned int wmi_ring_size = 3;
	uint64_t secs, usecs;
	struct wmi_event_debug *wmi_evt;
	struct wmi_unified *wmi_handle;
	struct wmi_log_buf_t *wmi_log;
	struct wmi_hang_data_fixed_param *cmd;
	struct wmi_command_debug *wmi_cmd;
	uint8_t *wmi_buf_ptr;

	if (!wmi_hang_data)
		return;

	wmi_handle = notif_block->priv_data;
	if (!wmi_handle)
		return;

	if (wmi_hang_data->offset >= QDF_WLAN_MAX_HOST_OFFSET)
		return;

	total_len = sizeof(struct wmi_hang_data_fixed_param);

	while (nread--) {
		switch (wmi_history) {
		case WMI_EVT_HIST:
			wmi_buf_ptr = (wmi_hang_data->hang_data +
				       wmi_hang_data->offset);
			cmd = ((struct wmi_hang_data_fixed_param *)wmi_buf_ptr);
			QDF_HANG_EVT_SET_HDR(&cmd->tlv_header,
					     HANG_EVT_TAG_WMI_EVT_HIST,
		  QDF_HANG_GET_STRUCT_TLVLEN(struct wmi_hang_data_fixed_param));
			cmd->time = secs;
			break;
		case WMI_CMD_HIST:
			wmi_buf_ptr = (wmi_hang_data->hang_data +
				       wmi_hang_data->offset);
			cmd = ((struct wmi_hang_data_fixed_param *)wmi_buf_ptr);
			QDF_HANG_EVT_SET_HDR(&cmd->tlv_header,
					     HANG_EVT_TAG_WMI_CMD_HIST,
		 QDF_HANG_GET_STRUCT_TLVLEN(struct wmi_hang_data_fixed_param));
			cmd->time = secs;
			break;
		}
		if (pos == 0)
			pos = wmi_ring_size - 1;
		else
			pos--;
		wmi_hang_data->offset += total_len;
	}
}

static int wmi_recovery_notifier_call(struct notifier_block *block,
				      unsigned long state,
				      void *data)
{
	wmi_log_history(block, data, WMI_EVT_HIST);
	wmi_log_history(block, data, WMI_CMD_HIST);

	return NOTIFY_OK;
}

static qdf_notif_block wmi_recovery_notifier = {
	.notif_block.notifier_call = wmi_recovery_notifier_call,
};

QDF_STATUS wmi_hang_event_notifier_register(struct wmi_unified *wmi_hdl)
{
	wmi_recovery_notifier.priv_data = wmi_hdl;
	return qdf_hang_event_register_notifier(&wmi_recovery_notifier);
}

QDF_STATUS wmi_hang_event_notifier_unregister(void)
{
	return qdf_hang_event_unregister_notifier(&wmi_recovery_notifier);
}
