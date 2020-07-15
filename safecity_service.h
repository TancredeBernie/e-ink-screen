#ifndef SAFECITY_SERVICE_H__
#define SAFECITY_SERVICE_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"

//TODO: modify
#define BLE_UUID_SF_BASE_UUID              {0x23, 0xD1, 0x13, 0xEF, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x33, 0x22, 0x11, 0x00} // 128-bit base UUID

//Defining 16-bit characteristic UUID
#define BLE_UUID_SF_SERVICE                		0xABCD // Just a random, but recognizable value
#define BLE_UUID_SF_CHARACTERISTC_UUID          0x1234 // Just a random, but recognizable value

typedef struct ble_sf_s ble_sf_t;

typedef void (*ble_sf_write_handler_t) (uint16_t conn_handle, ble_sf_t * p_sf, uint8_t data);

struct ble_sf_s
{
		uint16_t    conn_handle;    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection).*/
    uint16_t    service_handle; /**< Handle of Safecity Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t    char_handles;
		ble_sf_write_handler_t char_write_handler;
};

void ble_sf_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
void sf_service_init(ble_sf_t * p_sf_service);
void sf_sensor_characteristic_update(ble_sf_t *p_sf_service, uint8_t *temperature_value);

#endif  /* _ SAFECITY_SERVICE_H__ */
