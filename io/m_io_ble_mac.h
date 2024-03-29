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

#ifndef __M_IO_BLE_MAC_H__
#define __M_IO_BLE_MAC_H__

#include <mstdlib/mstdlib_io.h>
#include "m_io_ble_int.h"

#import <CoreBluetooth/CoreBluetooth.h>

typedef struct {
	M_io_handle_t  *handle;
	CFTypeRef       peripheral; /* CBPeripheral */
	/* key = service uuid
 	 * val = hash_strvp
	 *     key = uuid
	 *     val = CBCharacteristic
	 * A list of CBCharacteristic's are in the CBService object.
	 * The CBPeripheral has a list of CBService's.
	 * We only need the CBCharacteristic for read and write and we're
	 * caching it so we don't have to traverse multiple lists to find
	 * the one we want. */
	M_hash_strvp_t *services;
} M_io_ble_device_t;

void M_io_ble_cbc_event_reset(void);
void M_io_ble_saw_device(const char *uuid, const char *name, const M_list_str_t *service_uuids);

void M_io_ble_device_update_name(const char *uuid, const char *name);
void M_io_ble_device_reap_seen(void);
void M_io_ble_device_cache_peripherial(CBPeripheral *peripheral);
void M_io_ble_device_set_connected(CBPeripheral *peripheral);
void M_io_ble_device_set_state(const char *uuid, M_io_state_t state, const char *error);
M_bool M_io_ble_device_waiting_connect(const char *uuid, const M_list_str_t *service_uuids);
M_io_error_t M_io_ble_device_write(const char *uuid, const char *service_uuid, const char *characteristic_uuid, const unsigned char *data, size_t data_len, M_bool blind);
M_io_error_t M_io_ble_device_req_val(const char *uuid, const char *service_uuid, const char *characteristic_uuid);
M_io_error_t M_io_ble_device_req_rssi(const char *uuid);
void M_io_ble_device_write_complete(const char *uuid, const char *service_uuid, const char *characteristic_uuid);
void M_io_ble_device_read_data(const char *uuid, const char *service_uuid, const char *characteristic_uuid, const unsigned char *data, size_t data_len);
void M_io_ble_device_read_rssi(const char *uuid, M_int64 rssi);
void M_io_ble_device_notify_done(const char *uuid, const char *service_uuid, const char *characteristic_uuid);

#endif /* __M_IO_BLE_MAC_H__ */
