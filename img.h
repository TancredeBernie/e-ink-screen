#include "stdint.h"

typedef struct {
    uint8_t  B;
    uint8_t  M;
    uint32_t fsize;
    uint16_t res1;
    uint16_t res2;
    uint32_t offset;
    uint32_t Bit_Pixel;
    uint32_t BMP_Width;
    uint32_t BMP_Height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t ctype;
    uint32_t dsize;
    uint32_t hppm;
    uint32_t vppm;
    uint32_t colorsused;
    uint32_t colorreq;
} BMP_HEADER;

extern const unsigned char gImage_1in54[];
extern const unsigned char gImage_warning[];
extern const unsigned char gImage_clock[];
