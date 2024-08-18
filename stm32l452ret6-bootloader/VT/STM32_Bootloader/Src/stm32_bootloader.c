/**
  ******************************************************************************
  * @file    stm32_bootloader.c
  * @author  trieunvt
  * @brief   STM32 bootloader module driver.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32_bootloader.h"

/* Private typedefs ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
uint8_t propertyVault[VAULT_HEIGHT][VAULT_WIDTH];
uint8_t currentAppVersion[VERSION_SIZE];
uint8_t newAppVersion[VERSION_SIZE];
uint8_t newApp[APP_HEIGHT][APP_WIDTH];
uint32_t currentAppBaseAddr;
uint32_t newAppBaseAddr;
uint32_t newAppOffset;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/**
  * @brief	Initialize STM32 bootloader.
  * @param	None.
  * @retval	None.
  */
void STM32_Bootloader_Init(void) {
	SEGGER_RTT_printf(0, "STM32 Bootloader Init\n");

	SEGGER_RTT_printf(0, "Bootloader base address: 0x%08x\n", FLASH_BASE);
	SEGGER_RTT_printf(0, "Bootloader version: %s\n", VERSION_BOOT);

	/* Load property vault from device flash */
	memset(propertyVault, 0xFF, sizeof(propertyVault));
	for (size_t i = 0; i < sizeof(propertyVault); i += VAULT_WIDTH) {
		memcpy((uint8_t *)propertyVault + i,
			   (uint32_t *)(VAULT_ADDRESS + i), VAULT_WIDTH);
	}

	/* Check current app slot */
	if (propertyVault[0][0] == APP_A) {
		currentAppBaseAddr = APP_ADDRESS_A;
		newAppBaseAddr = APP_ADDRESS_B;
		memcpy(currentAppVersion, propertyVault[1], VERSION_SIZE);
	} else if (propertyVault[0][0] == APP_B) {
		currentAppBaseAddr = APP_ADDRESS_B;
		newAppBaseAddr = APP_ADDRESS_A;
		memcpy(currentAppVersion, propertyVault[2], VERSION_SIZE);
	} else {
		/* Set up for first ever boot */
		*(uint64_t *)(propertyVault[0]) = APP_A;
		*(uint64_t *)(propertyVault[1]) = 0x000001;
		*(uint64_t *)(propertyVault[2]) = 0x000000;

		if (STM32_Bootloader_Flash((uint8_t *)propertyVault, VAULT_ADDRESS,
								   sizeof(propertyVault)) == IS_FALSE) {
			SEGGER_RTT_printf(0, "Failed to set up for first ever boot\n");
			STM32_Bootloader_Jump(APP_ADDRESS_A);
		}

		currentAppBaseAddr = APP_ADDRESS_A;
		newAppBaseAddr = APP_ADDRESS_B;
		memcpy(currentAppVersion, propertyVault[1], VERSION_SIZE);
	}

	SEGGER_RTT_printf(0, "Current app slot: %d\n", propertyVault[0][0]);
	SEGGER_RTT_printf(0, "Current app base address: 0x%08x\n", currentAppBaseAddr);
	SEGGER_RTT_printf(0, "Current app version: %02x.%02x.%02x\n",
		currentAppVersion[0], currentAppVersion[1], currentAppVersion[2]);
}

/**
  * @brief	DeInitialize STM32 bootloader.
  * @param	None.
  * @retval	None.
  */
void STM32_Bootloader_DeInit(void) {
	SEGGER_RTT_printf(0, "STM32 Bootloader DeInit\n");

	HAL_RCC_DeInit();
	HAL_DeInit();

	/* Disable System Fault Handler */
	SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk |\
					SCB_SHCSR_BUSFAULTENA_Msk |\
					SCB_SHCSR_MEMFAULTENA_Msk);
}

/**
  * @brief	Process STM32 bootloader.
  * @param	None.
  * @retval	None.
  */
void STM32_Bootloader_Process(void) {
	SEGGER_RTT_printf(0, "STM32 Bootloader Process\n");

	uint8_t command[COMMAND_LENGTH];
	uint8_t buffer[BUFFER_SIZE];
	uint32_t timeout = HAL_GetTick() + BOOT_TIMEOUT;

	/* Check boot timeout */
	while (HAL_GetTick() < timeout) {
		/* Get boot command */
		if (HAL_UART_Receive(&huart3, command, COMMAND_LENGTH, UART_TIMEOUT)) {
			continue;
		}

		/* Reset timeout and wait for boot command */
		timeout = HAL_GetTick() + BOOT_TIMEOUT;

		memset(buffer, 0, BUFFER_SIZE);
		strncpy((char *)buffer, (char *)command, COMMAND_LENGTH);
		SEGGER_RTT_printf(0, "Boot command: %s\n", buffer);

		/* Compare boot command with ALL command */
		if (!strncmp((char *)command, COMMAND_ALL, COMMAND_LENGTH)) {
			/* Send property vault data */
			memset(buffer, 0, BUFFER_SIZE);
			sprintf((char *)buffer, "%08x\n", propertyVault[0][0]);
			sprintf((char *)buffer + 1 * (VAULT_WIDTH + 1), "%02x.%02x.%02x\n",
					propertyVault[1][0], propertyVault[1][1], propertyVault[1][2]);
			sprintf((char *)buffer + 2 * (VAULT_WIDTH + 1), "%02x.%02x.%02x\n",
					propertyVault[2][0], propertyVault[2][1], propertyVault[2][2]);

			HAL_Delay(UART_DELAY);
			HAL_UART_Transmit(&huart3, buffer,
							  VAULT_HEIGHT * (VAULT_WIDTH + 1), UART_TIMEOUT);
			continue;
		}

		/* Compare boot command with DFU command */
		if (strncmp((char *)command, COMMAND_DFU, COMMAND_LENGTH)) {
			continue;
		}

		/* Confirm boot command */
		HAL_Delay(UART_DELAY);
		if (HAL_UART_Transmit(&huart3, (uint8_t *) "OK",
							  strlen("OK"), UART_TIMEOUT)) {
			SEGGER_RTT_printf(0, "Failed to confirm boot command\n");
			continue;
		}

		/* Get new app version */
		/* Use "+2" to handle "\r\n" of DFU version frame */
		memset(buffer, 0, BUFFER_SIZE);
		if (HAL_UART_Receive(&huart3, buffer,
							 strlen(VERSION_BOOT) + 2, UART_TIMEOUT)) {
			continue;
		}

		SEGGER_RTT_printf(0, "New app version: %s\n", buffer);
		for (size_t i = 0; i < VERSION_SIZE; i++) {
			newAppVersion[i] = strtol((char *)buffer + 3 * i, NULL, 16);
		}

		/* Compare new app version with current app version */
		if (strncmp((char *)newAppVersion,
					(char *)currentAppVersion, VERSION_SIZE) <= 0) {
			HAL_Delay(UART_DELAY);
			HAL_UART_Transmit(&huart3, (uint8_t *) "NO",
							  strlen("NO"), UART_TIMEOUT);
			SEGGER_RTT_printf(0, "New app version is not greater than current app version\n");
			continue;
		}

		/* Confirm new app version */
		HAL_Delay(UART_DELAY);
		if (HAL_UART_Transmit(&huart3, (uint8_t *) "OK",
							  strlen("OK"), UART_TIMEOUT)) {
			SEGGER_RTT_printf(0, "Failed to confirm new app version\n");
			continue;
		}

		/* Update new app */
		if (STM32_Bootloader_DFU() == IS_FALSE) {
			SEGGER_RTT_printf(0, "Failed to update new app\n");
			break;
		}
	}

	SEGGER_RTT_printf(0, "Process time out\n");
	STM32_Bootloader_Jump(currentAppBaseAddr);
}
