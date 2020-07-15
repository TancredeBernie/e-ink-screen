#include "stdlib.h"
#include "nrf.h"
//#include "bsp.h"
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"
#include "SEGGER_RTT.h"
#include "nrf_log_default_backends.h"

bool readFile(const char* file_name, char** buf);
bool writeFile(const char* file_name, char* buf);
bool readBmp(const char* file_name, uint8_t* img);
bool sd_card_init(void);
