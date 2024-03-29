/* The MIT License (MIT)
 * 
 * Copyright (c) 2018 Monetra Technologies, LLC.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __M_IO_BLE_INT_H__
#define __M_IO_BLE_INT_H__

#include <mstdlib/mstdlib.h>
#include <mstdlib/io/m_io_layer.h>
#include "m_io_meta.h"

#define M_IO_BLE_NAME "BLE"

typedef enum {
	M_IO_BLE_META_KEY_UNKNOWN = 0,
	M_IO_BLE_META_KEY_SERVICE_UUID,        /* str */
	M_IO_BLE_META_KEY_CHARACTERISTIC_UUID, /* str */
	M_IO_BLE_META_KEY_WRITE_TYPE,          /* int (M_io_ble_wtype_t) */
	M_IO_BLE_META_KEY_READ_TYPE,           /* int (M_io_ble_rtype_t) */
	M_IO_BLE_META_KEY_RSSI,                /* int */
	M_IO_BLE_META_KEY_NOTIFY               /* bool */
} M_io_ble_meta_keys;

typedef struct {
	char          identifier[256];
	char          name[256];
	M_list_str_t *service_uuids;
	M_time_t      last_seen;
} M_io_ble_enum_device_t;

struct M_io_ble_enum {
	M_list_t *devices;
};

typedef struct {
	M_io_ble_rtype_t  type;
	union {
		struct {
			M_int64    val;
		} rssi;
		struct {
			char       service_uuid[256];
			char       characteristic_uuid[256];
			M_buf_t   *data;
		} read;
		struct {
			char       service_uuid[256];
			char       characteristic_uuid[256];
		} notify;
	} d;
} M_io_ble_rdata_t;

typedef struct {
	char service_uuid[256];
	char characteristic_uuid[256];
} M_io_ble_wcomplete_t;

struct M_io_handle {
	M_io_t           *io;                /*!< io object handle is associated with. */
	char              uuid[256];         /*!< UUID of device in use. */
	char              service_uuid[256]; /*!< UUID of service used for connecting using service. */
	M_llist_t        *read_queue;        /*!< List of M_io_ble_rdata_t objects with data that has been read. */
	M_list_t         *wcomplete_queue;   /*!< List of M_io_ble_wcomplete_t object with write event identifiers. */
	M_event_timer_t  *timer;             /*!< Timer to handle connection timeouts */
	M_event_timer_t  *initalized_timer;  /*!< Timer to retry connecting when waiting for initialization*/
	M_uint64          timeout_ms;        /*!< Timeout for connecting. */
	char              error[256];        /*!< Error message. */
	M_io_state_t      state;
	M_bool            can_write;         /*!< Wether data can be written. Will be false if a write operation is processing. */
	M_bool            have_max_write;
	size_t            max_write_w_response;
	size_t            max_write_wo_response;
};

M_io_ble_enum_t *M_io_ble_enum_init(void);
void M_io_ble_enum_add(M_io_ble_enum_t *btenum, const M_io_ble_enum_device_t *edev);
M_uint64 M_io_ble_validate_timeout(M_uint64 timeout_ms);
void M_io_ble_enum_free_device(M_io_ble_enum_device_t *dev);

M_bool M_io_ble_initalized(void);
M_bool M_io_ble_init_int(void);
void M_io_ble_connect(M_io_handle_t *handle);
void M_io_ble_close(M_io_handle_t *handle);

void M_io_ble_rdata_destroy(M_io_ble_rdata_t *d);
M_bool M_io_ble_rdata_queue_add_read(M_llist_t *queue, const char *service_uuid, const char *characteristic_uuid, const unsigned char *data, size_t data_len);
M_bool M_io_ble_rdata_queue_add_rssi(M_llist_t *queue, M_int64 rssi);
M_bool M_io_ble_rdata_queue_add_notify(M_llist_t *queue, const char *service_uuid, const char *characteristic_uuid);

void M_io_ble_wcomplete_destroy(M_io_ble_wcomplete_t *wc);
void *M_io_ble_wcomplete_duplicate(M_io_ble_wcomplete_t *wc);
int M_io_ble_wcomplete_compar(const void *arg1, const void *arg2, void *thunk);
M_bool M_io_ble_wcomplete_queue_push(M_list_t *write_resp_uuids, const char *service_uuid, const char *characteristic_uuid);
M_bool M_io_ble_wcomplete_queue_pop(M_list_t *write_resp_uuids, char **service_uuid, char **characteristic_uuid);

/* Used by ble.c but not in the file. */

M_io_error_t M_io_ble_set_device_notify(const char *uuid, const char *service_uuid, const char *characteristic_uuid, M_bool enable);
char *M_io_ble_get_device_identifier(const char *uuid);
char *M_io_ble_get_device_name(const char *uuid);
M_list_str_t *M_io_ble_get_device_services(const char *uuid);
M_list_str_t *M_io_ble_get_device_service_characteristics(const char *uuid, const char *service_uuid);
M_io_ble_property_t M_io_ble_get_device_service_characteristic_properties(const char *uuid, const char *service_uuid, const char *characteristic_uuid);
void M_io_ble_get_device_max_write_sizes(const char *uuid, size_t *with_response, size_t *without_response);

M_io_handle_t *M_io_ble_open(const char *uuid, M_io_error_t *ioerr, M_uint64 timeout_ms);
M_io_handle_t *M_io_ble_open_with_service(const char *service_uuid, M_io_error_t *ioerr, M_uint64 timeout_ms);
M_bool M_io_ble_errormsg_cb(M_io_layer_t *layer, char *error, size_t err_len);
M_io_state_t M_io_ble_state_cb(M_io_layer_t *layer);
void M_io_ble_destroy_cb(M_io_layer_t *layer);
M_bool M_io_ble_process_cb(M_io_layer_t *layer, M_event_type_t *type);
M_io_error_t M_io_ble_write_cb(M_io_layer_t *layer, const unsigned char *buf, size_t *write_len, M_io_meta_t *meta);
M_io_error_t M_io_ble_read_cb(M_io_layer_t *layer, unsigned char *buf, size_t *read_len, M_io_meta_t *meta);
void M_io_ble_unregister_cb(M_io_layer_t *layer);
M_bool M_io_ble_disconnect_cb(M_io_layer_t *layer);
M_bool M_io_ble_init_cb(M_io_layer_t *layer);

#endif
