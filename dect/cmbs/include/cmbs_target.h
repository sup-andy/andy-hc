/*!
* \file     cmbs_target.h
* \brief  It contains the relevant HW information for Cordless Module Base(CMBS)
* \Author  kelbch
*
*   This file contains the relevant information and changes between the DSPG
*   Development board and the CMBS UART/IOM Module.
*   The UART IOM module uses following GPIOs:
*   UART    Base GPIO   7
*   IOM/PCM Base GPIO   2
*   IIC Bus      GPIO   0,1
*
*******************************************************************************
* \par History
* \n==== History ============================================================\n
* date   name  version  action                                          \n
* ----------------------------------------------------------------------------\n
*  05-Mar-09   D.Kelbch  1.0      Initialize
*******************************************************************************/

#if !defined( CMBS_TARGET_H )
#define CMBS_TARGET_H

#if defined (CMBS_HW_MODULE)
/* The CMBS_UART IOM module uses according schematics
   GPIO 7,8,9,10 for UART communication
   GPIO 2,3,4,5  for PCM/IOM communication
*/

#define DR18_IOM_GPIO 2
#if defined(USE_SPI_TRANSPORT)
#define DR18_GPIO_UART 0
#else
#if defined (DCX81_MOD_UART)
#define DR18_GPIO_UART 9
#else
#define DR18_GPIO_UART 7
#endif
#endif

#else
#if defined(USE_SPI_TRANSPORT)
#define DR18_GPIO_UART 0
#else
#define DR18_GPIO_UART 22
#endif
#endif



/** RF Rx Feedback GPIO setup Start */
/**********************************/
#if defined(DCX78_DEV_UART) || defined(DCX78_DEV_USB) || (DCX78_DEV_SPI)
/* DCX78 Dev board */
#define    LM16_MOD_I_MONITOR_PIN    26
#define    LM16_RX_DATA_INPUT_PIN    14
#else
#if defined(DCX78_MOD_UART) || defined(DCX78_MOD_USB) || (DCX78_MOD_SPI)
/* DCX7x module board with SPI connectivity */
#define    LM16_MOD_I_MONITOR_PIN    11
#define    LM16_RX_DATA_INPUT_PIN    25
#else
/* Default setup */
#define    LM16_MOD_I_MONITOR_PIN    26
#define    LM16_RX_DATA_INPUT_PIN    13
#endif
#endif
/**********************************/
/** RF Rx Feedback GPIO setup End */

/* */

#if defined( __cplusplus )
extern "C"
{
#endif

#if defined( __cplusplus )
}
#endif

#endif // CMBS_TARGET_H
//*/
