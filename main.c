/***************************************************************************//**
* \file main.c
* \version 1.0
*
* Main source file of the FX10 device Hello World application.
*
*******************************************************************************
* \copyright
* (c) (2021-2023), Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.
*
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include "cy_pdl.h"
#include <string.h>
#include "app_version.h"
#include "cy_debug.h"
#include "cybsp.h"

/* Select SCB interface used for UART based logging. */
#define LOGGING_SCB             (SCB1)
#define LOGGING_SCB_IDX         (1u)
#define DEBUG_LEVEL             (3u)

#if DEBUG_INFRA_EN
/* Debug log related initilization */
#define LOGBUF_SIZE (1024u)
uint8_t logBuff[LOGBUF_SIZE];
#if USBFS_LOGS_ENABLE
    cy_stc_debug_config_t dbgCfg = {logBuff, DEBUG_LEVEL, LOGBUF_SIZE, CY_DEBUG_INTFCE_USBFS_CDC, true};
#else
    cy_stc_debug_config_t dbgCfg = {logBuff, DEBUG_LEVEL, LOGBUF_SIZE, CY_DEBUG_INTFCE_UART_SCB1, true};
#endif /* USBFS_LOGS_ENABLE */
#endif /* DEBUG_INFRA_EN */

/*****************************************************************************
 * Function Name: PrintVersionInfo
 ******************************************************************************
 * Summary:
 *  Function to print version information to UART console.
 *
 * Parameters:
 *  type: Type of version string.
 *  typeLen: Length of version type string.
 *  vMajor: Major version number (0 - 99)
 *  vMinor: Minor version number (0 - 99)
 *  vPatch: Patch version number (0 - 99)
 *  vBuild: Build number (0 - 9999)
 *
 * Return:
 *  None
 *****************************************************************************/
void PrintVersionInfo (const char *type, uint8_t typeLen,
                       uint8_t vMajor, uint8_t vMinor, uint8_t vPatch, uint16_t vBuild)
{
    DBG_APP_INFO("%s%d.%d.%d.%d\r\n", type, vMajor, vMinor, vPatch, vBuild);
}

/*******************************************************************************
 * Function name: Cy_Fx3g2_InitPeripheralClocks
 ****************************************************************************//**
 *
 * Function used to enable clocks to different peripherals on the FX10/FX20 device.
 *
 * \param adcClkEnable
 * Whether to enable clock to the ADC in the USBSS block.
 *
 * \param usbfsClkEnable
 * Whether to enable bus reset detect clock input to the USBFS block.
 *
 *******************************************************************************/
void Cy_Fx3g2_InitPeripheralClocks (
        bool adcClkEnable,
        bool usbfsClkEnable)
{
    if (adcClkEnable) {
        /* Divide PERI clock at 75 MHz by 75 to get 1 MHz clock using 16-bit divider #1. */
        Cy_SysClk_PeriphSetDivider(CY_SYSCLK_DIV_16_BIT, 1, 74);
        Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_16_BIT, 1);
        Cy_SysLib_DelayUs(10U);
        Cy_SysClk_PeriphAssignDivider(PCLK_LVDS2USB32SS_CLOCK_SAR, CY_SYSCLK_DIV_16_BIT, 1);
    }

    if (usbfsClkEnable) {
        /* Divide PERI clock at 75 MHz by 750 to get 100 KHz clock using 16-bit divider #2. */
        Cy_SysClk_PeriphSetDivider(CY_SYSCLK_DIV_16_BIT, 2, 749);
        Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_16_BIT, 2);
        Cy_SysLib_DelayUs(10U);
        Cy_SysClk_PeriphAssignDivider(PCLK_USB_CLOCK_DEV_BRS, CY_SYSCLK_DIV_16_BIT, 2);
    }
}

/*****************************************************************************
 * Function Name: PeripheralInit
 *****************************************************************************
 * Summary
 *  Initialize peripherals used by the application.
 *
 * Parameters:
 *  None
 *
 * Return:
 *  void
 ****************************************************************************/
void PeripheralInit (void)
{
    cy_stc_gpio_pin_config_t pinCfg;

    /* sequence */
    Cy_Fx3g2_InitPeripheralClocks(true, true);

    /* Configure the GPIOs used for firmware activity indication. */
    memset ((void *)&pinCfg, 0, sizeof(pinCfg));
    pinCfg.driveMode = CY_GPIO_DM_STRONG_IN_OFF;
    pinCfg.hsiom     = P0_0_GPIO;
    Cy_GPIO_Pin_Init(P0_0_PORT, P0_0_PIN, &pinCfg);
    pinCfg.hsiom     = P0_1_GPIO;
    Cy_GPIO_Pin_Init(P0_1_PORT, P0_1_PIN, &pinCfg);
}

/*****************************************************************************
* Function Name: main(void)
******************************************************************************
* Summary:
*  Entry to the application.
*
* Parameters:
*  void

* Return:
*  Does not return.
*****************************************************************************/
int main(void)
{
    const char start_string[] = "***** FX20: Hello World Application *****\r\n";
    uint32_t loopCount = 0;

    /* Initialize the PDL driver library and set the clock variables. */
    Cy_PDL_Init(&cy_deviceIpBlockCfgFX3G2);

    /* Do all the relevant clock configuration at start-up. */
    cybsp_init();

    /* Perform PDL and UART initialization as the first step. */
    PeripheralInit();

    /* Unlock and then disable the watchdog. */
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    /* Enable all interrupts. */
    __set_BASEPRI(0);

    /* Enable interrupts. */
    __enable_irq();

#if !USBFS_LOGS_ENABLE
    /* Initialize the UART for logging. */
    InitUart(LOGGING_SCB_IDX);

#endif /* USBFS_LOGS_ENABLE */
    /*
     * Initialize the logger module. We are using a blocking print option which will
     * output the messages on UART immediately.
     */
    Cy_Debug_LogInit(&dbgCfg);

    Cy_SysLib_Delay(500);

    /* Print start-up string using SCB0 - UART. */
    DBG_APP_INFO("%s", start_string);

    /* Print application and USBD stack version information. */
    PrintVersionInfo("APP_VERSION: ", 13, APP_VERSION_MAJOR, APP_VERSION_MINOR,
            APP_VERSION_PATCH, APP_VERSION_BUILD);

    while (1) {
        /* Toggle P0.0, print UART message, toggle P0.1 and then wait for 1 second. */
        Cy_GPIO_Inv(P0_0_PORT, P0_0_PIN);
        DBG_APP_INFO("Completed %d loops\r\n", loopCount++);
        Cy_GPIO_Inv(P0_1_PORT, P0_1_PIN);
        Cy_SysLib_Delay(1000);
    }

    /* Return statement will not be hit. */
    return 0;
}

/* [] END OF FILE */
