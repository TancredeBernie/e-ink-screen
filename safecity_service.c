#include <stdint.h>
#include <string.h>
#include "safecity_service.h"
#include "ble_srv_common.h"
#include "app_error.h"
#include "SEGGER_RTT.h"

static void on_write(ble_sf_t * p_sf_service, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if((p_evt_write->handle == p_sf_service->char_handles.value_handle) &&
        (p_evt_write->len == 1) &&
        (p_sf_service->char_write_handler != NULL))
    {
        p_sf_service->char_write_handler(p_ble_evt->evt.gap_evt.conn_handle, p_sf_service, p_evt_write->data[0]);
    }
}


void ble_sf_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
  	ble_sf_t * p_sf_service =(ble_sf_t *) p_context;  
		switch (p_ble_evt->header.evt_id)
		{
				case BLE_GAP_EVT_CONNECTED:
					p_sf_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
					break;
				case BLE_GAP_EVT_DISCONNECTED:
					p_sf_service->conn_handle = BLE_CONN_HANDLE_INVALID;
					break;
				case BLE_GATTS_EVT_WRITE:
            on_write(p_context, p_ble_evt);
            break;
				default:
        // No implementation needed.
        break;
		}
}

static uint32_t sf_char_add(ble_sf_t * p_sf_service)
{
		uint32_t            err_code;
		ble_uuid_t          char_uuid;
		ble_uuid128_t       base_uuid = BLE_UUID_SF_BASE_UUID;
		char_uuid.uuid      = BLE_UUID_SF_CHARACTERISTC_UUID;
		err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
		APP_ERROR_CHECK(err_code);  
    
    // Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));
		char_md.char_props.read = 1;
		char_md.char_props.write = 1;
    
    // Configuring Client Characteristic Configuration Descriptor metadata and add to char_md structure
    ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
		cccd_md.vloc                = BLE_GATTS_VLOC_STACK;    
		char_md.p_cccd_md           = &cccd_md;
		char_md.char_props.notify   = 1;
    
    // Configure the attribute metadata
    ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md));  
		attr_md.vloc = BLE_GATTS_VLOC_STACK;
    
    // Set read/write security levels to our characteristic
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    // Configure the characteristic value attribute
    ble_gatts_attr_t    attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));
		attr_char_value.p_uuid      = &char_uuid;
		attr_char_value.p_attr_md   = &attr_md;
    
    // Set characteristic length in number of bytes
		attr_char_value.max_len     = 2;
		attr_char_value.init_len    = 2;
		uint8_t value[2]            = {0x00,0x00};
		attr_char_value.p_value     = value;

    // Add our new characteristic to the service
		err_code = sd_ble_gatts_characteristic_add(p_sf_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_sf_service->char_handles);
		SEGGER_RTT_printf(0, "ADDING CHARACT SF error=%d\n", err_code);
		APP_ERROR_CHECK(err_code);

    return NRF_SUCCESS;
}

void sf_service_init(ble_sf_t * p_sf_service)
{  
    uint32_t   err_code;
		ble_uuid_t        service_uuid;
		ble_uuid128_t     base_uuid = BLE_UUID_SF_BASE_UUID;
		service_uuid.uuid = BLE_UUID_SF_SERVICE;
		//service_uuid.type = BLE_UUID_TYPE_VENDOR_BEGIN;
		//init handle service
		p_sf_service->conn_handle = BLE_CONN_HANDLE_INVALID;
	
		err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
		SEGGER_RTT_printf(0,"ERROR=%d\n",err_code);
		APP_ERROR_CHECK(err_code);
		// Add our service
		err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
																			&service_uuid,
																			&p_sf_service->service_handle);
		APP_ERROR_CHECK(err_code);
	
		//Call the function our_char_add() to add our new characteristic to the service. 
    sf_char_add(p_sf_service);
}

