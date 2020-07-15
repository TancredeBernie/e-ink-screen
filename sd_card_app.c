#include "sd_card_app.h"
#include "img.h"

bool writeFile(const char* file_name, char* buf){
    static FIL file;
    uint32_t bytes_written;
    FRESULT ff_result;

   // ouvrir avec les fonctionnalitées ecritures et écrasement à chaque fois qu'on écrit dedans
    ff_result = f_open(&file, file_name, FA_WRITE | FA_CREATE_ALWAYS);
	SEGGER_RTT_printf(0, "FFResult :%d \n" ,ff_result);
	//Verification de la creation du fichier 
    if (ff_result != FR_OK)
    {
			SEGGER_RTT_printf(0, "Unable to create file: %s. \n" ,file_name);
        return false;
    }

		// ecriture dans le fichier
    ff_result = f_write(&file, buf, strlen(buf), (UINT *) &bytes_written);
    if (ff_result != FR_OK)
    {
			SEGGER_RTT_printf(0, "Write failed\r\n.");
			return false;
    }
    else
    {
			SEGGER_RTT_printf(0, "%d bytes written.", bytes_written);
    }

    (void) f_close(&file);
    return true;
}


bool readFile(const char* file_name, char** buf){
    static FIL file;
    uint32_t bytes_readen;
    FRESULT ff_result;
	//ouverture du fichier en mode simplement lecture
	SEGGER_RTT_printf(0, " \n Read to file : %s ... \n" ,file_name);
	
    ff_result = f_open(&file, file_name, FA_READ );

    if (ff_result != FR_OK)
    {
			SEGGER_RTT_printf(0, "Unable to open : %s. \n" ,file_name);
			return false;
    }
 
		*buf = (char*)malloc(f_size(&file));
		//lecture du fichier et stockage dans un tableau
		ff_result = f_read(&file, *buf, f_size(&file), (UINT *) &bytes_readen);

    if (ff_result != FR_OK)
    {
			SEGGER_RTT_printf(0, "read failed\r\n.");
			return false;
    }
    else
    {
			SEGGER_RTT_printf(0, "%d bytes readen.", bytes_readen);
    }
			//SEGGER_RTT_printf(0, "%s \n ", *bufferFichier);
		
    (void) f_close(&file);
		
    return true;
}

bool readBmp(const char* file_name, uint8_t* img) {
		static FIL file;
    uint32_t bytes_readen;
    FRESULT ff_result;
		uint8_t BMP_Headinfo[sizeof(BMP_HEADER)];//52
    BMP_HEADER* pbmpheader = (BMP_HEADER*)BMP_Headinfo;
	
		SEGGER_RTT_printf(0, " \n Read BMP file : %s ... \n" ,file_name);
	
    ff_result = f_open(&file, file_name, FA_READ );

    if (ff_result != FR_OK)
    {
			SEGGER_RTT_printf(0, "Unable to open : %s. \n" ,file_name);
			return false;
    }
		
		//lecture du header bmp
		ff_result = f_read(&file, BMP_Headinfo, sizeof(BMP_HEADER), (UINT *) &bytes_readen);

    if (ff_result != FR_OK)
    {
			SEGGER_RTT_printf(0, "read failed\r\n.");
			return false;
    }
    else
    {
			SEGGER_RTT_printf(0, "%d bytes readen.", bytes_readen);
    }
		
		//check bmp
		if(pbmpheader->B == 0x4D && pbmpheader->M == 0x42) { //0x4d42
        SEGGER_RTT_printf(0, "the file is not bmp\r\n");
        return false;
    }
		SEGGER_RTT_printf(0, "file size =  %d \r\n", pbmpheader->fsize);
    SEGGER_RTT_printf(0, "file offset =  %d \r\n", pbmpheader->offset);
		
		ff_result = f_lseek(&file, pbmpheader->offset);
		
		if (ff_result != FR_OK)
    {
			SEGGER_RTT_printf(0, "seek failed\r\n.");
			return false;
    }
		
		//lecture des données BMP
		uint32_t dataSize = f_size(&file) - pbmpheader->offset;
		//test si dataSize > size of img
		ff_result = f_read(&file, img, dataSize, (UINT *) &bytes_readen);
		
		if (ff_result != FR_OK)
    {
			SEGGER_RTT_printf(0, "read failed\r\n.");
			return false;
    }
    else
    {
			SEGGER_RTT_printf(0, "%d bytes readen.", bytes_readen);
    }
		
		(void) f_close(&file);
		
    return true;
}

bool sd_card_init(){
	//initialisation des pins de la carte SD
	NRF_BLOCK_DEV_SDC_DEFINE(
        m_block_dev_sdc,
        NRF_BLOCK_DEV_SDC_CONFIG(
                SDC_SECTOR_SIZE,
                APP_SDCARD_CONFIG(SPI_MOSI_PIN, SPI_MISO_PIN, SPI_SCK_PIN, SPI_SD_CS_PIN)
         ),
         NFR_BLOCK_DEV_INFO_CONFIG("Nordic", "SDC", "1.00")
);

    static FATFS fs;
    FRESULT ff_result;
    DSTATUS disk_state = STA_NOINIT;

    // Initialize FATFS disk I/O interface by providing the block device.
    static diskio_blkdev_t drives[] =
    {
            DISKIO_BLOCKDEV_CONFIG(NRF_BLOCKDEV_BASE_ADDR(m_block_dev_sdc, block_dev), NULL)
    };

    diskio_blockdev_register(drives, ARRAY_SIZE(drives));

   
		//initialisation du disque
		SEGGER_RTT_printf(0, "Initializing disk 0 (SDC)... \n");
    for (uint32_t retries = 3; retries && disk_state; --retries)
    {
        disk_state = disk_initialize(0);
    }
    if (disk_state)
    {
			SEGGER_RTT_printf(0, "Disk initialization failed.\n");
        return false;
    }

    // montage du volume et de la capacité de la carte SD
    uint32_t blocks_per_mb = (1024uL * 1024uL) / m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_size;
    uint32_t capacity = m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_count / blocks_per_mb;
		SEGGER_RTT_printf(0, "Capacity: %d MB \n" , capacity);
    SEGGER_RTT_WriteString(0,"Mounting volume...");
		SEGGER_RTT_printf(0, "Mounting volume... \n");
    ff_result = f_mount(&fs, "", 1);
    if (ff_result)
    {
				SEGGER_RTT_printf(0, "Mount failed. \n");
        return false;
    }
		return true;

}	
