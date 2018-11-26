#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "zbar.h"

/* 内部函数实现 */
static int ALIGN(int x, int y) {
    // y must be a power of 2.
    return (x + y - 1) & ~(y - 1);
}

#pragma pack(1)
typedef struct {
    uint16_t  bfType;
    uint32_t  bfSize;
    uint16_t  bfReserved1;
    uint16_t  bfReserved2;
    uint32_t  bfOffBits;
    uint32_t  biSize;
    uint32_t  biWidth;
    uint32_t  biHeight;
    uint16_t  biPlanes;
    uint16_t  biBitCount;
    uint32_t  biCompression;
    uint32_t  biSizeImage;
    uint32_t  biXPelsPerMeter;
    uint32_t  biYPelsPerMeter;
    uint32_t  biClrUsed;
    uint32_t  biClrImportant;
} BMPFILEHEADER;
#pragma pack()

/* to complete a runnable example, this abbreviated implementation of
 * get_data() will use libpng to read an image file. refer to libpng
 * documentation for details
 */
static void get_data(const char *name,
                     int *width, int *height,
                     void **raw)
{
    BMPFILEHEADER header = {0};
    FILE         *fp     = NULL;
    uint8_t      *pdata  = NULL;
    int           stride, i, j, k;

    fp = fopen(name, "rb");
    if (!fp) return;

    fread(&header, sizeof(header), 1, fp);
    *width  = header.biWidth;
    *height = header.biHeight;
    stride  = ALIGN(header.biWidth * 3, 4);
    pdata   = malloc(header.biWidth * header.biWidth);
    *raw    = pdata;
    if (pdata) {
        pdata += header.biWidth * header.biWidth;
        for (i=0; i<header.biHeight; i++) {
            pdata -= header.biWidth;
            for (j=0; j<header.biWidth; j++) {
                int r = fgetc(fp);
                int g = fgetc(fp);
                int b = fgetc(fp);
                pdata[j] = (r + g + b) / 3;
            }
            for (k=header.biWidth*3; k<stride; k++) {
                fgetc(fp);
            }
        }
    }

    fclose(fp);
}

int main(int argc, char **argv)
{
    zbar_image_scanner_t *scanner = NULL;
    zbar_image_t         *image   = NULL;
    const zbar_symbol_t  *symbol  = NULL;
    int  width = 0, height = 0, n;
    void *raw = NULL;

    /* create a reader */
    scanner = zbar_image_scanner_create();

    /* configure the reader */
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);

    /* obtain image data */
    get_data(argv[1], &width, &height, &raw);
    printf("width  = %d\n", width );
    printf("height = %d\n", height);

    /* wrap image data */
    image = zbar_image_create();
    zbar_image_set_format(image, *(int*)"Y800");
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, raw, width * height, zbar_image_free_data);

    /* scan the image for barcodes */
    n = zbar_scan_image(scanner, image);

    /* extract results */
    symbol = zbar_image_first_symbol(image);
    for(; symbol; symbol = zbar_symbol_next(symbol)) {
        /* do something useful with results */
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
        printf("decoded %s symbol \"%s\"\n",
               zbar_get_symbol_name(typ), data);
    }

    /* clean up */
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);
}

