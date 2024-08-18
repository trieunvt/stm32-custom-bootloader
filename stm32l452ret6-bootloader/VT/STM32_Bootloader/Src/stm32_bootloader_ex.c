/**
  ******************************************************************************
  * @file    stm32_bootloader_ex.c
  * @author  trieunvt
  * @brief   Extended STM32 bootloader module driver.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32_bootloader_ex.h"

/* Private typedefs ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/**
  * @brief	Jump to app.
  * @param	address	App address.
  * @retval	None.
  */
void STM32_Bootloader_Jump(uint32_t address) {
	SEGGER_RTT_printf(0, "Jump to app address: 0x%08x\n", address);
	STM32_Bootloader_DeInit();

	/* Initialize app Main Stack Pointer */
	__set_MSP(*((volatile uint32_t*)address));

	/* Set Vector Table Offset Register to address */
	SCB->VTOR = address;

	/* Set Program Counter to point to app */
	void (*jumpToAddress)(void) = (void*)(*((volatile uint32_t*)(address + 4UL)));

	/* Call function to jump to app */
	jumpToAddress();

	/* Should never get here! */
	SEGGER_RTT_printf(0, "Should never get here!\n");
	NVIC_SystemReset();

	while (1) {
		/* Do nothing */
	}
}

/**
  * @brief	Check sum of DFU frame.
  * @param	frame	DFU frame.
  * @param	length	Length of DFU frame.
  * @retval	Boolean.
  */
uint8_t STM32_Bootloader_Checksum(uint8_t *frame, uint16_t length) {
	uint16_t sum = 0;

	for (size_t i = 0; i < length - 1; i++) {
		sum += *(frame + i);
	}

	/* Calculate sum based on Intel HEX file format */
	uint8_t calcSum = ~(sum & 0xFF) + 0x01;
	uint8_t frameSum = *(frame + length - 1);

	return calcSum == frameSum;
}

/**
  * @brief	Parse data of DFU frame.
  * @param	frame	DFU frame.
  * @retval	None.
  */
void STM32_Bootloader_Parse(uint8_t *frame) {
	uint8_t frameRecordType = *(frame + 3);

	/* Parse DFU frame with data record type (0x00) */
	if (frameRecordType == 0x00) {
		uint8_t frameByteCount = *frame;
		uint8_t frameAddrHeight = *(frame + 1);
		uint8_t frameAddrWidth = *(frame + 2);
		uint8_t *frameData = (uint8_t *)(frame + 4);

		/* Empty memory for next frame data */
		/* Standard DFU frame has 16 byte data */
		/* Use "& 0xF0" to handle 2 types of DFU frames with just 8 byte data */
		memset(newApp[frameAddrHeight] + (frameAddrWidth & 0xF0) + FRAME_DATA_SIZE,
			   0xFF, FRAME_DATA_SIZE);

		/* Store latest frame address as new app offset */
		newAppOffset = (frameAddrHeight << 8) + frameAddrWidth + FRAME_DATA_SIZE;

		/* Store frame data as new app */
		for (size_t i = 0; i < frameByteCount; i++) {
			newApp[frameAddrHeight][frameAddrWidth + i] = *(frameData + i);
		}
	}
}

/**
  * @brief	Program data to device flash.
  * @param	data	Data pointer.
  * @param	address	Data base address.
  * @param	offset	Data address offset.
  * @retval	Boolean.
  */
uint8_t STM32_Bootloader_Flash(uint8_t *data, uint32_t address, uint32_t offset) {
	// SEGGER_RTT_printf(0, "Data start address: 0x%08x\n", address);
	// SEGGER_RTT_printf(0, "Data end address: 0x%08x\n", address + offset);

	/* Print data */
	// SEGGER_RTT_printf(0, "Data:\n");
	// uint8_t buffer[BUFFER_SIZE];
	// for (uint16_t i = 0; i < offset; i += FRAME_DATA_SIZE) {
	// 	for (size_t j = 0; j < FRAME_DATA_SIZE; j++) {
	// 		sprintf((char *)(buffer + j * strlen("FF ")), "%02x ",
	// 				*(data + i + j));
	// 	}

	// 	SEGGER_RTT_printf(0, "[%04d] %s\n", i / FRAME_DATA_SIZE, buffer);
	// }

	/* Set up flash erase variable */
	FLASH_EraseInitTypeDef flashErase;
	uint32_t flashError;
	HAL_StatusTypeDef status;

	flashErase.TypeErase = FLASH_TYPEERASE_PAGES;
	flashErase.Page = (address - FLASH_BASE) / FLASH_PAGE_SIZE;
	flashErase.NbPages = offset / FLASH_PAGE_SIZE + 1;

	/* Erase device flash */
	HAL_FLASH_Unlock();
	for (size_t i = 0; i < FLASH_RETRY; i++) {
		status = HAL_FLASHEx_Erase(&flashErase, &flashError);
		if (!status) break;
		SEGGER_RTT_printf(0, "Failed to erase flash page(s) with error code: %d\n",
							  HAL_FLASH_GetError());
	}

	/* Program data to device flash */
	if (!status) {
		uint64_t doubleWord;
		for (uint16_t i = 0; i < offset; i += sizeof(uint64_t)) {
			memcpy(&doubleWord, data + i, sizeof(uint64_t));

			for (size_t j = 0; j < FLASH_RETRY; j++) {
				status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
										   address + i, doubleWord);
				if (!status) break;
				SEGGER_RTT_printf(0, "Failed to program flash page(s) at address: 0x%08x\n",
									  address + i);
			}

			/* Failed to program packet to device flash many times */
			if (status) break;
		}
	}

	HAL_FLASH_Lock();
	return status == HAL_OK;
}

/**
  * @brief	Implement device firmware update (DFU).
  * @param	None.
  * @retval	Boolean.
  */
uint8_t STM32_Bootloader_DFU(void) {
	SEGGER_RTT_printf(0, "Implement device firmware update (DFU)\n");

	uint8_t frame[FRAME_SIZE];
	uint8_t byte[2];
	uint16_t frameCount = 0;
	uint16_t count = 0;
	uint8_t isInFrame = IS_FALSE;
	uint8_t isFullyReceived = IS_FALSE;
	uint32_t timeout = HAL_GetTick() + BOOT_TIMEOUT;

	/* Check DFU timeout */
	while (HAL_GetTick() < timeout) {
		/* Get UART data */
		if (HAL_UART_Receive(&huart3, byte, 1, UART_TIMEOUT)) {
			continue;
		}

		/* Reset timeout and wait for DFU frame */
		timeout = HAL_GetTick() + BOOT_TIMEOUT;

		/* Check start byte of DFU frame */
		if (!strncmp((char *)byte, ":", 1)) {
			isInFrame = IS_TRUE;
			continue;
		}

		/* Ignore "\r" end byte of DFU frame */
		if (!strncmp((char *)byte, "\r", 1)) {
			continue;
		}

		/* Check end byte of DFU frame */
		if (isInFrame && !strncmp((char *)byte, "\n", 1)) {
			if (STM32_Bootloader_Checksum(frame, count) == IS_FALSE) {
				SEGGER_RTT_printf(0, "Calculated sum is not equal frame sum\n");
				return IS_FALSE;
			}

			STM32_Bootloader_Parse(frame);
			HAL_Delay(UART_DELAY);

			/* Confirm DFU frame */
			if (HAL_UART_Transmit(&huart3, (uint8_t *) "OK",
								  strlen("OK"), UART_TIMEOUT)) {
				SEGGER_RTT_printf(0, "Failed to confirm DFU frame\n");
				return IS_FALSE;
			}

			/* Check device is still running */
			if (!(++frameCount % BUFFER_SIZE)) {
				SEGGER_RTT_printf(0, ".");
			}

			/* Check final DFU frame based on Intel HEX file format (:00000001FF) */
			if (frame[0] == 0x00 && frame[count - 1] == 0xFF) {
				SEGGER_RTT_printf(0, "\nTotal DFU frame(s) received: %d\n",
									  frameCount);
				isFullyReceived = IS_TRUE;
				break;
			}

			/* Reset data in DFU frame */
			memset(frame, 0xFF, FRAME_SIZE);
			isInFrame = IS_FALSE;
			count = 0;
			continue;
		}

		/* Check data bytes of DFU frame */
		if (isInFrame && !HAL_UART_Receive(&huart3, byte + 1, 1, UART_TIMEOUT)) {
			frame[count] = strtol((char *)byte, NULL, 16);
			count++;
			continue;
		}
	}

	if (!isFullyReceived) {
		SEGGER_RTT_printf(0, "DFU time out\n");
		return IS_FALSE;
	}

	SEGGER_RTT_printf(0, "Program new app to device flash\n");
	if (STM32_Bootloader_Flash((uint8_t *)newApp,
							   newAppBaseAddr, newAppOffset) == IS_FALSE) {
		SEGGER_RTT_printf(0, "Failed to program new app to device flash\n");
		return IS_FALSE;
	}

	SEGGER_RTT_printf(0, "Update property vault in device flash\n");
	if (newAppBaseAddr == APP_ADDRESS_A) {
		*(uint64_t *)(propertyVault[0]) = APP_A;
		memcpy(propertyVault[1], newAppVersion, VERSION_SIZE);
	} else if (newAppBaseAddr == APP_ADDRESS_B) {
		*(uint64_t *)(propertyVault[0]) = APP_B;
		memcpy(propertyVault[2], newAppVersion, VERSION_SIZE);
	} else {
		SEGGER_RTT_printf(0, "New app base address not found\n");
		return IS_FALSE;
	}

	if (STM32_Bootloader_Flash((uint8_t *)propertyVault,
							   VAULT_ADDRESS, sizeof(propertyVault)) == IS_FALSE) {
		SEGGER_RTT_printf(0, "Failed to update property vault in device flash\n");
		return IS_FALSE;
	}

	/* Reset device to run new app */
	NVIC_SystemReset();

	/* Should never get here! */
	STM32_Bootloader_Jump(newAppBaseAddr);
	return IS_TRUE;
}
