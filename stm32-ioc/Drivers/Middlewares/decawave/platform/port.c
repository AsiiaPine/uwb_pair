/*! ----------------------------------------------------------------------------
 * @file    port.c
 * @brief   HW specific definitions and functions for portability
 *
 * @attention
 *
 * Copyright 2016 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */

#include "port.h"
#include "deca_device_api.h"
#include "stm32f1xx_hal_conf.h"


/****************************************************************************//**
 *
 *                              APP global variables
 *
 *******************************************************************************/
extern SPI_HandleTypeDef hspi1;


/****************************************************************************//**
 *
 *                  Port private variables and function prototypes
 *
 *******************************************************************************/
static volatile uint32_t signalResetDone;

/****************************************************************************//**
 *
 *                              Time section
 *
 *******************************************************************************/

/* @fn    portGetTickCnt
 * @brief wrapper for to read a SysTickTimer, which is incremented with
 *        CLOCKS_PER_SEC frequency.
 *        The resolution of time32_incr is usually 1/1000 sec.
 * */
__INLINE uint32_t
portGetTickCnt(void)
{
    return HAL_GetTick();
}


/* @fn    usleep
 * @brief precise usleep() delay
 * */
#pragma GCC optimize ("O0")
int usleep(uint32_t usec)
{
#pragma GCC ivdep
    for(uint32_t i=0;i<usec;i++)
    {
#pragma GCC ivdep
        for(int j=0;j<2;j++)
        {
            __NOP();
            __NOP();
        }
    }
    return 0;
}


/* @fn    Sleep
 * @brief Sleep delay in ms using SysTick timer
 * */
__INLINE void
Sleep(uint32_t x)
{
    HAL_Delay(x);
}

/****************************************************************************//**
 *
 *                              END OF Time section
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                              Configuration section
 *
 *******************************************************************************/

/* @fn    peripherals_init
 * */
int peripherals_init (void)
{
    /* All has been initialized in the CubeMx code, see main.c */
    return 0;
}


/* @fn    spi_peripheral_init
 * */
void spi_peripheral_init()
{

    /* SPI's has been initialized in the CubeMx code, see main.c */

}



/**
  * @brief  Checks whether the specified EXTI line is enabled or not.
  * @param  EXTI_Line: specifies the EXTI line to check.
  *   This parameter can be:
  *     @arg EXTI_Linex: External interrupt line x where x(0..19)
  * @retval The "enable" state of EXTI_Line (SET or RESET).
  */
ITStatus EXTI_GetITEnStatus(uint32_t x)
{
    return ((NVIC->ISER[(((uint32_t)x) >> 5UL)] &\
            (uint32_t)(1UL << (((uint32_t)x) & 0x1FUL)) ) == (uint32_t)RESET)?(RESET):(SET);
}
/****************************************************************************//**
 *
 *                          End of configuration section
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                          DW1000 port section
 *
 *******************************************************************************/

/* @fn      reset_DW1000
 * @brief   DW_RESET pin on DW1000 has 2 functions
 *          In general it is output, but it also can be used to reset the digital
 *          part of DW1000 by driving this pin low.
 *          Note, the DW_RESET pin should not be driven high externally.
 * */
void reset_DW1000(void)
{
    GPIO_InitTypeDef    GPIO_InitStruct;

    // Enable GPIO used for DW1000 reset as open collector output
    GPIO_InitStruct.Pin = DW_RESET_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DW_RESET_GPIO_Port, &GPIO_InitStruct);

    //drive the RSTn pin low
    HAL_GPIO_WritePin(DW_RESET_GPIO_Port, DW_RESET_Pin, GPIO_PIN_RESET);

    usleep(1);

    //put the pin back to output open-drain (not active)
    setup_DW1000RSTnIRQ(0);



    Sleep(2);
}

/* @fn      setup_DW1000RSTnIRQ
 * @brief   setup the DW_RESET pin mode
 *          0 - output Open collector mode
 *          !0 - input mode with connected EXTI0 IRQ
 * */
void setup_DW1000RSTnIRQ(int enable)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    if(enable)
    {
        // Enable GPIO used as DECA RESET for interrupt
        GPIO_InitStruct.Pin = DW_RESET_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(DW_RESET_GPIO_Port, &GPIO_InitStruct);

        HAL_NVIC_EnableIRQ(EXTI0_IRQn);     //pin #0 -> EXTI #0
        HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    }
    else
    {
        HAL_NVIC_DisableIRQ(EXTI0_IRQn);    //pin #0 -> EXTI #0

        //put the pin back to tri-state ... as
        //output open-drain (not active)
        GPIO_InitStruct.Pin = DW_RESET_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(DW_RESET_GPIO_Port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(DW_RESET_GPIO_Port, DW_RESET_Pin, GPIO_PIN_SET);
    }
}


/* @fn      port_is_boot1_low
 * @brief   check the BOOT1 pin status.
 * @return  1 if ON and 0 for OFF
 * */
int port_is_boot1_low(void)
{
    return ((GPIO_ReadInputDataBit(TA_BOOT1_GPIO, TA_BOOT1))?(0):(1));
}

/* @fn      port_is_boot1_low
 * @brief   check the BOOT1 pin status.
 * @return  1 if ON and 0 for OFF
 * */
int port_is_boot1_on(uint16_t x)
{
    (void)x;
    return ((GPIO_ReadInputDataBit(TA_BOOT1_GPIO, TA_BOOT1))?(0):(1));
}

/* @fn      port_is_switch_on
 * @brief   check the switch status.
 *          when switch (S1) is 'on' the pin is low
 * @return  1 if ON and 0 for OFF
 * */
int port_is_switch_on(uint16_t GPIOpin)
{
    return ((GPIO_ReadInputDataBit(TA_SW1_GPIO, GPIOpin))?(0):(1));
}


/* @fn      led_off
 * @brief   switch off the led from led_t enumeration
 * */
void led_off(led_t led) {
    switch (led) {
    case LED_PC6:
        GPIO_ResetBits(LED1_GPIO_Port, LED1_Pin);
        break;
    case LED_PC7:
        GPIO_ResetBits(LED2_GPIO_Port, LED2_Pin);
        break;
    case LED_PC8:
        GPIO_ResetBits(LED1_GPIO_Port, LED1_Pin);
        break;
    case LED_PC9:
        GPIO_ResetBits(LED2_GPIO_Port, LED2_Pin);
        break;
    case LED_ALL:
        GPIO_ResetBits(LED1_GPIO_Port, LED1_Pin);
        GPIO_ResetBits(LED2_GPIO_Port, LED2_Pin);
        break;
    default:
        // do nothing for undefined led number
        break;
    }
}

/* @fn      led_on
 * @brief   switch on the led from led_t enumeration
 * */
void led_on (led_t led)
{
    switch (led)
    {
    case LED_PC6:
        GPIO_SetBits(LED1_GPIO_Port, LED1_Pin);
        break;
    case LED_PC7:
        GPIO_SetBits(LED2_GPIO_Port, LED2_Pin);
        break;
    case LED_PC8:
        GPIO_SetBits(LED1_GPIO_Port, LED1_Pin);
        break;
    case LED_PC9:
        GPIO_SetBits(LED2_GPIO_Port, LED2_Pin);
        break;
    case LED_ALL:
        GPIO_SetBits(LED1_GPIO_Port, LED1_Pin);
        GPIO_SetBits(LED2_GPIO_Port, LED2_Pin);
        break;
    default:
        // do nothing for undefined led number
        break;
    }
}


/* @fn      port_wakeup_dw1000
 * @brief   "slow" waking up of DW1000 using DW_CS only
 * */
void port_wakeup_dw1000(void)
{
    HAL_GPIO_WritePin(DW_NSS_GPIO_Port, DW_NSS_Pin, GPIO_PIN_RESET);
    Sleep(1);
    HAL_GPIO_WritePin(DW_NSS_GPIO_Port, DW_NSS_Pin, GPIO_PIN_SET);
    Sleep(7);                       //wait 7ms for DW1000 XTAL to stabilise
}

/* @fn      port_wakeup_dw1000_fast
 * @brief   waking up of DW1000 using DW_CS and DW_RESET pins.
 *          The DW_RESET signalling that the DW1000 is in the INIT state.
 *          the total fast wakeup takes ~2.2ms and depends on crystal startup time
 * */
void port_wakeup_dw1000_fast(void)
{
    #define WAKEUP_TMR_MS   (10)

    uint32_t x = 0;
    uint32_t timestamp = HAL_GetTick(); //protection

    setup_DW1000RSTnIRQ(0);         //disable RSTn IRQ
    signalResetDone = 0;            //signalResetDone connected to RST_PIN_IRQ
    setup_DW1000RSTnIRQ(1);         //enable RSTn IRQ
    port_SPIx_clear_chip_select();  //CS low

    //need to poll to check when the DW1000 is in the IDLE, the CPLL interrupt is not reliable
    //when RSTn goes high the DW1000 is in INIT, it will enter IDLE after PLL lock (in 5 us)
    while((signalResetDone == 0) && \
          ((HAL_GetTick() - timestamp) < WAKEUP_TMR_MS))
    {
        x++;     //when DW1000 will switch to an IDLE state RSTn pin will high
    }
    setup_DW1000RSTnIRQ(0);         //disable RSTn IRQ
    port_SPIx_set_chip_select();    //CS high

    //it takes ~35us in total for the DW1000 to lock the PLL, download AON and go to IDLE state
    usleep(35);
}



/* @fn      port_set_dw1000_slowrate
 * @brief   set 2.25MHz
 *          note: hspi1 is clocked from 72MHz
 * */
void port_set_dw1000_slowrate(void)
{
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    HAL_SPI_Init(&hspi1);
}

/* @fn      port_set_dw1000_fastrate
 * @brief   set 18MHz
 *          note: hspi1 is clocked from 72MHz
 * */
void port_set_dw1000_fastrate(void)
{
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    HAL_SPI_Init(&hspi1);
}



/****************************************************************************//**
 *
 *                          End APP port section
 *
 *******************************************************************************/



/****************************************************************************//**
 *
 *                              IRQ section
 *
 *******************************************************************************/

/* @fn      HAL_GPIO_EXTI_Callback
 * @brief   IRQ HAL call-back for all EXTI configured lines
 *          i.e. DW_RESET_Pin and DW_IRQn_Pin
 * */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == DW_RESET_Pin)
    {
        signalResetDone = 1;
    }
    else if (GPIO_Pin == DW_IRQn_Pin)
    {
        process_deca_irq();
    }
    else
    {
    }
}

/* @fn      process_deca_irq
 * @brief   main call-back for processing of DW1000 IRQ
 *          it re-enters the IRQ routing and processes all events.
 *          After processing of all events, DW1000 will clear the IRQ line.
 * */
__INLINE void process_deca_irq(void)
{
    while(port_CheckEXT_IRQ() != 0)
    {

        port_deca_isr();

    } //while DW1000 IRQ line active
}


/* @fn      port_DisableEXT_IRQ
 * @brief   wrapper to disable DW_IRQ pin IRQ
 *          in current implementation it disables all IRQ from lines 5:9
 * */
__INLINE void port_DisableEXT_IRQ(void)
{
    NVIC_DisableIRQ(DECAIRQ_EXTI_IRQn);
}

/* @fn      port_EnableEXT_IRQ
 * @brief   wrapper to enable DW_IRQ pin IRQ
 *          in current implementation it enables all IRQ from lines 5:9
 * */
__INLINE void port_EnableEXT_IRQ(void)
{
    NVIC_EnableIRQ(DECAIRQ_EXTI_IRQn);
}


/* @fn      port_GetEXT_IRQStatus
 * @brief   wrapper to read a DW_IRQ pin IRQ status
 * */
__INLINE uint32_t port_GetEXT_IRQStatus(void)
{
    return EXTI_GetITEnStatus(DECAIRQ_EXTI_IRQn);
}


/* @fn      port_CheckEXT_IRQ
 * @brief   wrapper to read DW_IRQ input pin state
 * */
__INLINE uint32_t port_CheckEXT_IRQ(void)
{
    return HAL_GPIO_ReadPin(DECAIRQ_GPIO, DW_IRQn_Pin);
}


/****************************************************************************//**
 *
 *                              END OF IRQ section
 *
 *******************************************************************************/



// /****************************************************************************//**
//  *
//  *                              USB report section
//  *
//  *******************************************************************************/
// #include "usb_device.h"

// #define REPORT_BUFSIZE  0x2000

// extern USBD_HandleTypeDef  hUsbDeviceFS;

// static struct
// {
//     HAL_LockTypeDef       Lock;     /*!< locking object                  */
// }
// txhandle={.Lock = HAL_UNLOCKED};

// static char     rbuf[REPORT_BUFSIZE];               /**< circular report buffer, data to be transmitted in flush_report_buff() Thread */
// static struct   circ_buf report_buf = { .buf = rbuf,
//                                         .head= 0,
//                                         .tail= 0};

// static uint8_t  ubuf[CDC_DATA_FS_MAX_PACKET_SIZE];  /**< used to transmit new chunk of data in single USB flush */

// /* @fn      port_tx_msg()
//  * @brief   put message to circular report buffer
//  *          it will be transmitted in background ASAP from flushing Thread
//  * @return  HAL_BUSY - can not do it now, wait for release
//  *          HAL_ERROR- buffer overflow
//  *          HAL_OK   - scheduled for transmission
//  * */
// HAL_StatusTypeDef port_tx_msg(uint8_t   *str, int  len)
// {
//     int head, tail, size;
//     HAL_StatusTypeDef   ret;

//     /* add TX msg to circular buffer and increase circular buffer length */

//     __HAL_LOCK(&txhandle);  //return HAL_BUSY if locked
//     head = report_buf.head;
//     tail = report_buf.tail;
//     __HAL_UNLOCK(&txhandle);

//     size = REPORT_BUFSIZE;

//     if(CIRC_SPACE(head, tail, size) > (len))
//     {
//         while (len > 0)
//         {
//             report_buf.buf[head]= *(str++);
//             head= (head+1) & (size - 1);
//             len--;
//         }

//         __HAL_LOCK(&txhandle);  //return HAL_BUSY if locked
//         report_buf.head = head;
//         __HAL_UNLOCK(&txhandle);

// #ifdef CMSIS_RTOS
//         osSignalSet(usbTxTaskHandle, signalUsbFlush);   //RTOS multitasking signal start flushing
// #endif
//         ret = HAL_OK;
//     }
//     else
//     {
//         /* if packet can not fit, setup TX Buffer overflow ERROR and exit */
//         ret = HAL_ERROR;
//     }

//     return ret;
// }


// /* @fn      flush_report_buff
//  * @brief   FLUSH should have higher priority than port_tx_msg()
//  *          it shall be called periodically from process, which can not be locked,
//  *          i.e. from independent high priority thread
//  * */
// HAL_StatusTypeDef flush_report_buff(void)
// {
//     USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*)(hUsbDeviceFS.pClassData);

//     int i, head, tail, len, size = REPORT_BUFSIZE;

//     __HAL_LOCK(&txhandle);  //"return HAL_BUSY;" if locked
//     head = report_buf.head;
//     tail = report_buf.tail;
//     __HAL_UNLOCK(&txhandle);

//     len = CIRC_CNT(head, tail, size);

//     if( len > 0 )
//     {
//         /*  check USB status - ready to TX */
//         if((hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) || (hcdc->TxState != 0))
//         {
//             return HAL_BUSY;    /**< USB is busy. Let it send now, will return next time */
//         }


//         /* copy MAX allowed length from circular buffer to non-circular TX buffer */
//         len = MIN(CDC_DATA_FS_MAX_PACKET_SIZE, len);

//         for(i=0; i<len; i++)
//         {
//             ubuf[i] = report_buf.buf[tail];
//             tail = (tail + 1) & (size - 1);
//         }

//         __HAL_LOCK(&txhandle);  //"return HAL_BUSY;" if locked
//         report_buf.tail = tail;
//         __HAL_UNLOCK(&txhandle);

//         /* setup USB IT transfer */
//         if(CDC_Transmit_FS(ubuf, (uint16_t)len) != USBD_OK)
//         {
//             /**< indicate USB transmit error */
//         }
//     }

//     return HAL_OK;
// }


/* DW1000 IRQ handler definition. */
port_deca_isr_t port_deca_isr = NULL;

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_set_deca_isr()
 *
 * @brief This function is used to install the handling function for DW1000 IRQ.
 *
 * NOTE:
 *   - As EXTI9_5_IRQHandler does not check that port_deca_isr is not null, the user application must ensure that a
 *     proper handler is set by calling this function before any DW1000 IRQ occurs!
 *   - This function makes sure the DW1000 IRQ line is deactivated while the handler is installed.
 *
 * @param deca_isr function pointer to DW1000 interrupt handler to install
 *
 * @return none
 */
void port_set_deca_isr(port_deca_isr_t deca_isr)
{
    /* Check DW1000 IRQ activation status. */
    ITStatus en = port_GetEXT_IRQStatus();

    /* If needed, deactivate DW1000 IRQ during the installation of the new handler. */
    if (en)
    {
        port_DisableEXT_IRQ();
    }
    port_deca_isr = deca_isr;
    if (en)
    {
        port_EnableEXT_IRQ();
    }
}


/****************************************************************************//**
 *
 *******************************************************************************/

