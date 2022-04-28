/*
 * sd_card.c
 *
 *  Created on: Apr 11, 2022
 *      Author: nickl
 */

#include "diskio.h"
#include "sd_card.h"

#define SD_CS_GPIO_Port GPIOA
#define SD_CS_Pin 4

extern SPI_HandleTypeDef hspi1;

static void select(){
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

static void deselect(){
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

static HAL_StatusTypeDef spi_txByte(uint8_t data){

	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
	return HAL_SPI_Transmit(&hspi1, &data, 1, SPI_TIMEOUT);
}

static uint8_t spi_rxByte(){
	uint8_t data = 0;
	uint8_t dummy = 0xFF;
	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
	HAL_SPI_TransmitReceive(&hspi1, &dummy, &data, 1, SPI_TIMEOUT);
	return data;
}

static uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg){
	uint8_t resp;

	if(cmd == CMD17 || cmd == CMD18 || cmd == CMD24 || cmd == CMD25 || cmd == CMD32 || cmd == CMD33) {
		arg = arg << 9;
	}

	select();
	spi_txByte(cmd | 0b01000000);
	spi_txByte(arg >> 24);
	spi_txByte(arg >> 16);
	spi_txByte(arg >> 8);
	spi_txByte(arg >> 0);

	if(cmd == CMD8){
		spi_txByte(0x87);
	} else {
		spi_txByte(0x95);
	}

	while ((resp = spi_rxByte()) == 0xff);
		if (resp == 0 && cmd == CMD58){
			uint8_t status = spi_rxByte();
			if(status == 0x40){
				SDHC_flag = 1;
			} else {
				SDHC_flag = 0;
			}
			spi_rxByte();
			spi_rxByte();
			spi_rxByte();
		}

	if(cmd == CMD38){
		while (spi_rxByte() == 0);
	}
	spi_rxByte();
	deselect();
	return resp;
}

DSTATUS SD_init(BYTE pdrv){
	uint8_t resp, retry;
	for(int i = 0; i<10; i++){
		spi_txByte(0xff);
	}
	select();

	retry=0;
	do {
		resp = SD_sendCommand(CMD0, 0);
		retry++;
		if(retry == 0xff){
			return 1;
		}
	} while (resp != 0x01);

	deselect();
	spi_txByte(0xff);
	spi_txByte(0xff);

	uint8_t version = 2;

	do {
		resp = SD_sendCommand(CMD8, 0x000001AA);
	} while (resp != 0x01);

	do {
		resp = SD_sendCommand(CMD55, 0);
		resp = SD_sendCommand(CMD41, 0x40000000);
	} while (resp != 0);

	SDHC_flag = 0;

	if (version == 2){
		do {
			resp = SD_sendCommand(CMD58, 0);
		} while (resp != 0);


	}

	resp = SD_sendCommand(CMD59, 0);
	return 0;
}

DSTATUS SD_status(BYTE pdrv){
	uint8_t resp = 0;
	uint8_t status;
	resp = SD_sendCommand(CMD13, 0);
	if(resp != 0){
		return resp;
	}
	select();
	status = spi_rxByte();
	deselect();
	return status;
}

DSTATUS SD_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count){
	uint8_t resp = 0;

	for (int j = 0; j<count; j++){
		resp = SD_sendCommand(CMD17, sector);

		if(resp != 0){
			return resp;
		}

		select();

		while(spi_rxByte() != TOKEN);

		for(int i = 0 ; i<BLOCK_SIZE; i++){
			*buff = spi_rxByte();
			buff++;
		}

		spi_rxByte();
		spi_rxByte();
	}
	spi_rxByte();

	deselect();

	return 0;
}
DSTATUS SD_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count){
	uint8_t resp = 0;

	for (int j = 0; j<count; j++){
		resp = SD_sendCommand(CMD24, sector);
		if(resp != 0){
			return resp;
		}
		select();
		spi_txByte(TOKEN);
		for(int i = 0 ; i<BLOCK_SIZE; i++){
			spi_txByte(*buff);
			buff++;
		}
		spi_txByte(0xFF);
		spi_txByte(0xFF);
		resp = spi_rxByte();

		if( (resp & 0b00011111) != 0b00000101){
			deselect();
			return resp;
		}

		while (spi_rxByte() == 0);
	}
	deselect();
	spi_txByte(0xFF);
	return 0;

}
DSTATUS SD_ioctl(BYTE pdrv, BYTE cmd, void* buff){
	return 0;
}

DSTATUS SD_erase(BYTE pdrv, DWORD start, UINT count){
	uint8_t resp;
	resp = SD_sendCommand(CMD32, start);
	if(resp != 0)
			return resp;
	resp = SD_sendCommand(CMD33, start+count-1);
	if(resp != 0)
				return resp;
	resp = SD_sendCommand(CMD38, 0);
	if(resp != 0)
				return resp;
}

DSTATUS SD_getCSD(BYTE pdrv){
	uint8_t resp = 0;

	resp = SD_sendCommand(CMD9, 0);

	if(resp != 0){
		return resp;
	}

	select();

	while(spi_rxByte() != TOKEN);

	for(int i = 0 ; i<16; i++){
		csd.reg[i] = spi_rxByte();
	}

	spi_rxByte();
	spi_rxByte();
	spi_rxByte();

	deselect();

	return 0;
}
