/*
 * bitmap_driver.h
 *
 *  Created on: 27. apr. 2022
 *      Author: nickl
 */

#ifndef INC_BITMAP_DRIVER_H_
#define INC_BITMAP_DRIVER_H_

#include "fatfs.h"
#include "stm32l4xx_hal.h"

typedef enum{
	BI_RGB,
	BI_RLE8,
	BI_RLE4,
	BI_BITFIELDS,
	BI_JPEG,
	BI_PNG,
	BI_ALPHABITFIELDS,
	BI_CMYK = 11,
	BI_CMYKRLE8,
	BI_CMYKRLE4
}CompressionType;


//pack ensures the struct is layed out with no padding, allowing easy conversion from raw byte array from file to the struct
#pragma pack(push,1)
typedef struct BitmapFileHeader{
	uint16_t headerField;
	uint32_t size;
	uint16_t res1;
	uint16_t res2;
	uint32_t offset;
}BitmapFileHeader;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct BitmapInfoHeader{
	uint32_t HeaderSize;
	int32_t width;
	int32_t height;
	uint16_t Ncolorplanes;
	uint16_t BitsPrPixel;
	uint32_t CompressionMethod;
	uint32_t ImageSize;
	uint32_t horizontalResolution;
	uint32_t verticalResolution;
	uint32_t NcolorPalette;
	uint32_t NimportantColors;
}BitmapInfoHeader;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct BitmapHeader{
	BitmapFileHeader bmfh;
	BitmapInfoHeader bmih;
}BitmapHeader;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct Pixel{
	uint8_t B;
	uint8_t G;
	uint8_t R;
}Pixel;
#pragma pack(pop)

BitmapHeader bm_getBitmapHeader(FIL* file, char* fileName);
uint32_t bm_getImageData(FIL* file, char* fileName, uint32_t offset, uint32_t Npixels, uint8_t* buffer);
void bm_fillImageBuffer(FIL* file, char* fileName, uint8_t* buffer);
#endif /* INC_BITMAP_DRIVER_H_ */
