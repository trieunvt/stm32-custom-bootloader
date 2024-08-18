/**
  ******************************************************************************
  * @file    stm32_bootloader_ex.h
  * @author  trieunvt
  * @brief   Header file of STM32 bootloader extended module.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32_BOOTLOADER_EX_H
#define STM32_BOOTLOADER_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32_bootloader.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void STM32_Bootloader_Jump(uint32_t address);
uint8_t STM32_Bootloader_Checksum(uint8_t *frame, uint16_t length);
void STM32_Bootloader_Parse(uint8_t *frame);
uint8_t STM32_Bootloader_Flash(uint8_t *data, uint32_t address, uint32_t offset);
uint8_t STM32_Bootloader_DFU(void);

/* External variables --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* STM32_BOOTLOADER_EX_H */
