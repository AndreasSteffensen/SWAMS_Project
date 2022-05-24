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
extern volatile uint8_t Timer1, Timer2;

static volatile DSTATUS Stat = STA_NOINIT;
static volatile uint8_t PowerFlag = 0;
static uint8_t CardType;
volatile uint8_t SDHC_flag;


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

static uint8_t SD_ReadyWait(void)
{
  uint8_t res;
  spi_rxByte();

  do
  {
    res = spi_rxByte();
  } while ((res != 0xFF));

  return res;
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

	while ((resp = spi_rxByte()) == 0xff){}
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

//get the Card-specifier data register, used for IOCTL for disk
DSTATUS SD_getCSD(BYTE pdrv, uint8_t* csd){
	uint8_t resp = 0;

	resp = SD_sendCommand(CMD9, 0);

	if(resp != 0){
		return resp;
	}

	select();

	while(spi_rxByte() != TOKEN);

	for(int i = 0 ; i<16; i++){
		csd[i] = spi_rxByte();
	}

	spi_rxByte();
	spi_rxByte();
	spi_rxByte();

	deselect();

	return 0;
}

static void SD_PowerOn(void)
{
  uint8_t cmd_arg[6];
  uint32_t Count = 0x1FFF;

  deselect();

  for(int i = 0; i < 10; i++)
  {
	  spi_txByte(0xFF);
  }

  select();

  cmd_arg[0] = (CMD0 | 0x40);
  cmd_arg[1] = 0;
  cmd_arg[2] = 0;
  cmd_arg[3] = 0;
  cmd_arg[4] = 0;
  cmd_arg[5] = 0x95;

  for (int i = 0; i < 6; i++)
  {
	  spi_txByte(cmd_arg[i]);
  }

  while ((spi_rxByte() != 0x01) && Count)
  {
    Count--;
  }

  deselect();
  spi_txByte(0XFF);

  PowerFlag = 1;
}

static void SD_PowerOff(void)
{
  PowerFlag = 0;
}

DSTATUS SD_init(BYTE drv) {
	BYTE n, success, ocr[4];

	// drv > 0 is multiple drives and not supported
	if (drv)
		return STA_NOINIT;

	SD_PowerOn();

	select();
	success = 0;
	//try and go into idle state
	if (SD_sendCommand(CMD0, 0) == 1) {
		Timer1 = 100; /* Initialization timeout of 1000 msec */
		if (SD_sendCommand(CMD8, 0x1AA) == 1) { /* SDC Ver2+ */
			for (n = 0; n < 4; n++)
				ocr[n] = spi_rxByte();
			if (ocr[1] == 0x01 && ocr[2] == 0xAA) { /* The card can work at vdd range of 2.7-3.6V */
				do {
					if (SD_sendCommand(CMD55, 0) <= 1
							&& SD_sendCommand(CMD41, 1UL << 30) == 0)
						break; /* ACMD41 with HCS bit */
				} while (Timer1);
				if (Timer1 && SD_sendCommand(CMD58, 0) == 0) { /* Check CCS bit */
					for (n = 0; n < 4; n++)
						ocr[n] = spi_rxByte();
					success = (ocr[0] & 0x40) ? 6 : 2;
				}
			}
		} else { /* SDC Ver1 or MMC */
			success = (SD_sendCommand(CMD55, 0) <= 1 && SD_sendCommand(CMD41, 0) <= 1) ? 2 : 1; /* SDC : MMC */
			do {
				if (success == 2) {
					if (SD_sendCommand(CMD55, 0) <= 1 && SD_sendCommand(CMD41, 0) == 0)
						break; /* ACMD41 */
				} else {
					if (SD_sendCommand(CMD1, 0) == 0)
						break; /* CMD1 */
				}
			} while (Timer1);
			if (!Timer1 || SD_sendCommand(CMD16, 512) != 0) /* Select R/W block length */
				success = 0;
		}
	}
	CardType = success;
	deselect();
	spi_rxByte();


	if (success)
		Stat &= ~STA_NOINIT;
	else
		/* Initialization failed */
		SD_PowerOff();

	return Stat;
}

DSTATUS SD_status(BYTE pdrv){
	  if (pdrv)
	    return STA_NOINIT;

	  return Stat;
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
	DRESULT res;
	BYTE n, csd[16], *ptr = buff;
	WORD csize;

	if (pdrv)
	return RES_PARERR;

	res = RES_ERROR;

	if (cmd == CTRL_POWER)
	{
		switch (*ptr)
		{
		case 0:
		  res = RES_OK;
		  break;
		case 1:
		  //SD_PowerOn();             /* Power On */
		  res = RES_OK;
		  break;
		case 2:
		  *(ptr + 1) = 1;
		  res = RES_OK;             /* Power Check */
		  break;
		default:
		  res = RES_PARERR;
		}
	}
	else
	{
		if (Stat & STA_NOINIT)
		  return RES_NOTRDY;

		select();

		switch (cmd)
		{
		case GET_SECTOR_COUNT:
		  if ((SD_sendCommand(CMD9, 0) == 0) && SD_getCSD(pdrv,csd))
		  {
			if ((csd[0] >> 6) == 1)
			{
			  csize = csd[9] + ((WORD) csd[8] << 8) + 1;
			  *(DWORD*) buff = (DWORD) csize << 10;
			}
			else
			{
			  n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
			  csize = (csd[8] >> 6) + ((WORD) csd[7] << 2) + ((WORD) (csd[6] & 3) << 10) + 1;
			  *(DWORD*) buff = (DWORD) csize << (n - 9);
			}

			res = RES_OK;
		  }
		  break;

		case GET_SECTOR_SIZE:
		  *(WORD*) buff = 512;
		  res = RES_OK;
		  break;

		case CTRL_SYNC:
		  if (SD_ReadyWait() == 0xFF)
			res = RES_OK;
		  break;

		case MMC_GET_CSD:
		  if (SD_sendCommand(CMD9, 0) == 0 && SD_getCSD(1, ptr))
			res = RES_OK;
		  break;

		case MMC_GET_CID:
		  if (SD_sendCommand(CMD10, 0) == 0 && SD_getCSD(1, ptr))
			res = RES_OK;
		  break;

		case MMC_GET_OCR:
		  if (SD_sendCommand(CMD58, 0) == 0)
		  {
			for (n = 0; n < 4; n++)
			{
			  *ptr++ = spi_rxByte();
			}

			res = RES_OK;
		  }

		default:
		  res = RES_PARERR;
		}

		deselect();
		spi_rxByte();
	}

	return res;
}

