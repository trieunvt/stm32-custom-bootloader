/**
  ******************************************************************************
  * @file    stm32_bootloader.h
  * @author  trieunvt
  * @brief   Header file of STM32 bootloader module.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32_BOOTLOADER_H
#define STM32_BOOTLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"
#include "string.h"
#include "stm32l452xx.h"
#include "stm32_bootloader_ex.h"
#include "SEGGER_RTT.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define BOOT_TIMEOUT    5000

#define VERSION_BOOT    "01.00.00"
#define VERSION_SIZE    sizeof(VERSION_BOOT) / 3

#define COMMAND_ALL     "ALL"
#define COMMAND_DFU     "DFU"
#define COMMAND_LENGTH  (COMMAND_DFU) - 1

#define VAULT_ADDRESS   + 0x10000UL
#define VAULT_HEIGHT    3
#define VAULT_WIDTH     sizeof(uint64_t)

#define APP_A           1
#define APP_B           2
#define APP_ADDRESS_A   FLASH_BASE + 0x20000UL
#define APP_ADDRESS_B   FLASH_BASE + 0x50000UL
#define APP_HEIGHT      256
#define APP_WIDTH       256

#define UART_TIMEOUT    50
#define UART_DELAY      20

#define FRAME_SIZE      45
#define FRAME_DATA_SIZE 16

#define BUFFER_SIZE     256

#define FLASH_RETRY     5

#define IS_TRUE         1
#define IS_FALSE        0

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void STM32_Bootloader_Init(void);
void STM32_Bootloader_DeInit(void);
void STM32_Bootloader_Process(void);

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart3;
extern uint8_t propertyVault[VAULT_HEIGHT][VAULT_WIDTH];
extern uint8_t currentAppVersion[VERSION_SIZE];
extern uint8_t newAppVersion[VERSION_SIZE];
extern uint8_t newApp[APP_HEIGHT][APP_WIDTH];
extern uint32_t currentAppBaseAddr;
extern uint32_t newAppBaseAddr;
extern uint32_t newAppOffset;

#ifdef __cplusplus
}
#endif

#endif /* STM32_BOOTLOADER_H */
