/*
 * sd_card.h
 *
 *  Created on: Apr 11, 2022
 *      Author: nickl
 */

#ifndef INC_SD_CARD_H_
#define INC_SD_CARD_H_

#include "fatfs.h"

#define CMD0 (0x40+0)
#define CMD1 (0x40+1)
#define CMD8 (0x40+8)
#define CMD9 (0x40+9)
#define CMD10 (0x40+10)
#define CMD12 (0x40+12)
#define CMD13 (0x40+13)
#define CMD16 (0x40+16)
#define CMD17 (0x40+17)
#define CMD18 (0x40+18)
#define CMD23 (0x40+23)
#define CMD24 (0x40+24)
#define CMD25 (0x40+25)
#define CMD32 (0x40+32)
#define CMD33 (0x40+33)
#define CMD38 (0x40+38)
#define CMD41 (0x40+41)
#define CMD55 (0x40+55)
#define CMD58 (0x40+58)
#define CMD59 (0x40+59)

#define BLOCK_SIZE 512
#define TOKEN 0b11111110

typedef struct CSDRegister{
	uint8_t reg[16];
} CSDRegister;

volatile CSDRegister csd;

volatile uint8_t SDHC_flag, cardType;

DSTATUS SD_init(BYTE pdrv);
DSTATUS SD_status(BYTE pdrv);
DSTATUS SD_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DSTATUS SD_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DSTATUS SD_ioctl(BYTE pdrv, BYTE cmd, void* buff);
DSTATUS SD_erase(BYTE pdrv, DWORD start, UINT count);
DSTATUS SD_getCSD(BYTE pdrv);
#define SPI_TIMEOUT 1000

#endif /* INC_SD_CARD_H_ */
