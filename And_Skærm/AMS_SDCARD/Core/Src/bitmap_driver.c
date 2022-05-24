/*
 * bitmap_driver.c
 *
 *  Created on: 27. apr. 2022
 *      Author: nickl
 */

#include "bitmap_driver.h"
#include "string.h"

uint8_t readBuffer[512];

BitmapHeader bm_getBitmapHeader(FIL* file, char* fileName){
	memset(readBuffer,0,512);
	uint32_t bytesRead;
	FRESULT res;
	res = f_open(file, fileName, FA_READ);
	res = f_read(file, readBuffer, 14 + 40, (UINT*)&bytesRead);

	BitmapFileHeader bmfh = *(BitmapFileHeader*)&readBuffer[0x00];
	BitmapInfoHeader bmih = *(BitmapInfoHeader*)&readBuffer[0x0E];

	f_close(file);

	BitmapHeader bmh = {bmfh,bmih};
	return bmh;
}

uint32_t bm_getImageData(FIL* file, char* fileName, uint32_t offset, uint32_t Npixels, uint8_t* buffer){
	//memset(readBuffer,0,512);
	uint32_t bytesRead;
	FRESULT res;
	res = f_open(file, fileName, FA_READ);
	res = f_lseek(file, 14 + 40 + offset);
	res = f_read(file, buffer, Npixels * 3, &bytesRead);
	f_close(file);

	return bytesRead;
}

void bm_fillImageBuffer(FIL* file, char* fileName, uint8_t* buffer){
	BitmapHeader header = getBitmapHeader(file,fileName);
	for (int i = header.bmih.height-1; i >= 0; i--){
		getImageData(file,fileName, i * header.bmih.width * 3, header.bmih.width , buffer);
		buffer += header.bmih.width*3;
	}
}
