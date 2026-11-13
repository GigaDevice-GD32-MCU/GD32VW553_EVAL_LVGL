/*!
    \file    lcd_driver.c
    \brief   lcd driver functions

    \version 2026-02-27, V1.6.0, demo for GD32VW55x
*/

/*
    Copyright (c) 2026, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32vw55x.h"
#include "lcd_driver.h"
#include "systick.h"

#ifdef H_VIEW
    #define X_MAX_PIXEL         (uint16_t)320
    #define Y_MAX_PIXEL         (uint16_t)240
#else
    #define X_MAX_PIXEL         (uint16_t)240
    #define Y_MAX_PIXEL         (uint16_t)320
#endif

#define LCD_DMA_PROBE_TIMEOUT   ((uint32_t)20000U)
#define LCD_DMA_TX_TIMEOUT      ((uint32_t)12000000U)
#define LCD_DMA_MAX_ONESHOT     ((uint32_t)0xFFFFU)

#define LCD_DMA_TX_CHANNEL     DMA_CH3
#define LCD_DMA_TX_SUBPERI     DMA_SUBPERI3

uint8_t spi_write_byte(uint8_t byte);
static void spi1_init(void);
static void lcd_write_index(uint8_t index);
static void lcd_write_data(uint8_t data);
static void lcd_write_data_16bit(uint8_t datah,uint8_t datal);
static void lcd_reset(void);
static void lcd_spi_drain_rx(void);
static void lcd_dma_init(void);

static void lcd_dma_apply_config(void);

static uint8_t lcd_dma_enabled = 0U;

/* ---- Async DMA state ---- */
static lcd_dma_complete_cb_t s_async_cb = NULL;
static void *s_async_user_data = NULL;
static const uint8_t *s_async_bytes = NULL;
static volatile uint32_t s_async_remaining = 0U;
static volatile uint8_t s_async_busy = 0U;

/*!
    \brief      send a byte through the SPI interface and return a byte received from the SPI bus
    \param[in]  byte: data to be send
    \param[out] none
    \retval     the value of the received byte
*/
uint8_t spi_write_byte(uint8_t byte)
{
    while(RESET == (SPI_STAT&SPI_FLAG_TBE));
    SPI_DATA = byte;

    while(RESET == (SPI_STAT&SPI_FLAG_RBNE));
    return(SPI_DATA);
} 

/*!
    \brief      initialize SPI0
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void spi1_init(void)
{
    spi_parameter_struct spi_init_struct;
    rcu_periph_clock_enable(RCU_SPI);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    /* configure SPI GPIO: SCK/PA11, MISO/PA10, MOSI/PA9 */
    gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);

    /* configure GPIOB */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_12 | GPIO_PIN_13);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_12 | GPIO_PIN_13);

    /* configure GPIOA */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_12);

    /* configure SPI parameter */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_4;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(&spi_init_struct);

    /* set crc polynomial */
    spi_crc_polynomial_set(7);
    spi_enable();

    lcd_dma_init();
}

/*! 
    \brief      drain SPI RX buffer to avoid stale RBNE data
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void lcd_spi_drain_rx(void)
{
    while(SET == spi_flag_get(SPI_FLAG_RBNE)) {
        (void)SPI_DATA;
    }
}

/*! 
    \brief      apply DMA base configuration for SPI TX
    \param[in]  channelx: DMA channel
    \param[in]  sub_periph: DMA sub peripheral select
    \param[out] none
    \retval     none
*/
static void lcd_dma_apply_config(void)
{
    dma_single_data_parameter_struct dma_init;

    dma_deinit(LCD_DMA_TX_CHANNEL);
    dma_single_data_para_struct_init(&dma_init);
    dma_init.periph_addr = (uint32_t)(&SPI_DATA);
    dma_init.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init.memory0_addr = 0U;
    dma_init.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_init.direction = DMA_MEMORY_TO_PERIPH;
    dma_init.number = 0U;
    dma_init.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(LCD_DMA_TX_CHANNEL, &dma_init);
    dma_channel_subperipheral_select(LCD_DMA_TX_CHANNEL, LCD_DMA_TX_SUBPERI);
    dma_channel_disable(LCD_DMA_TX_CHANNEL);
    dma_flag_clear(LCD_DMA_TX_CHANNEL,
                   DMA_FLAG_FTF | DMA_FLAG_HTF | DMA_FLAG_TAE | DMA_FLAG_SDE | DMA_FLAG_FEE);
}

static void lcd_dma_init(void)
{
    rcu_periph_clock_enable(RCU_DMA);
    lcd_dma_apply_config();
    dma_interrupt_enable(LCD_DMA_TX_CHANNEL, DMA_INT_FTF);
    eclic_irq_enable(DMA_Channel3_IRQn, 2, 1);
    lcd_dma_enabled = 1U;
}

/*!
    \brief      write the register address
    \param[in]  index: the value of register address to be written
    \param[out] none
    \retval     none
*/
static void lcd_write_index(uint8_t index)
{
    LCD_RS_CLR;
    spi_write_byte(index);
}

/*!
    \brief      write the register data
    \param[in]  data: the value of register data to be written
    \param[out] none
    \retval     none
*/
static void lcd_write_data(uint8_t data)
{
    LCD_RS_SET;
    spi_write_byte(data);
}

/*!
    \brief      write the register data(an unsigned 16-bit data)
    \param[in]  datah: the high 8bit of register data to be written
    \param[in]  datal: the low 8bit of register data to be written
    \param[out] none
    \retval     none
*/
static void lcd_write_data_16bit(uint8_t datah,uint8_t datal)
{
    lcd_write_data(datah);
    lcd_write_data(datal);
}

/*!
    \brief      reset the lcd
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void lcd_reset(void)
{
    LCD_RST_CLR;
    delay_1ms(100);
    LCD_RST_SET;
    delay_1ms(50);
}

/*!
    \brief      initialize the lcd
    \param[in]  none
    \param[out] none
    \retval     none
*/
void lcd_init(void)
{
    spi1_init();

    LCD_CS_CLR;
    lcd_reset();

    /* write the register address 0xCB */
    lcd_write_index(0xCB);
    lcd_write_data(0x39);
    lcd_write_data(0x2C);
    lcd_write_data(0x00);
    lcd_write_data(0x34);
    lcd_write_data(0x02);

    /* write the register address 0xCF */
    lcd_write_index(0xCF);
    lcd_write_data(0x00);
    lcd_write_data(0XC1);
    lcd_write_data(0X30);

    /* write the register address 0xE8 */
    lcd_write_index(0xE8);
    lcd_write_data(0x85);
    lcd_write_data(0x00);
    lcd_write_data(0x78);

    /* write the register address 0xEA */
    lcd_write_index(0xEA);
    lcd_write_data(0x00);
    lcd_write_data(0x00);

    /* write the register address 0xED */
    lcd_write_index(0xED);
    lcd_write_data(0x64);
    lcd_write_data(0x03);
    lcd_write_data(0X12);
    lcd_write_data(0X81);

    /* write the register address 0xF7 */
    lcd_write_index(0xF7);
    lcd_write_data(0x20);

    /* power control VRH[5:0] */
    lcd_write_index(0xC0);
    lcd_write_data(0x23);

    /* power control SAP[2:0];BT[3:0] */
    lcd_write_index(0xC1);
    lcd_write_data(0x10);

    /* vcm control */
    lcd_write_index(0xC5);
    lcd_write_data(0x3e);
    lcd_write_data(0x28); 

    /* vcm control2 */
    lcd_write_index(0xC7);
    lcd_write_data(0x86);

    lcd_write_index(0x36);
#ifdef H_VIEW
    lcd_write_data(0xE8);
#else
    lcd_write_data(0x48); 
#endif
    /* write the register address 0x3A */
    lcd_write_index(0x3A);
    lcd_write_data(0x55);

    /* write the register address 0xB1 */
    lcd_write_index(0xB1);
    lcd_write_data(0x00);
    lcd_write_data(0x18);

    /* display function control */
    lcd_write_index(0xB6);
    lcd_write_data(0x08); 
    lcd_write_data(0x82);
    lcd_write_data(0x27);  

    /* 3gamma function disable */
    lcd_write_index(0xF2);
    lcd_write_data(0x00); 

    /* gamma curve selected  */
    lcd_write_index(0x26);
    lcd_write_data(0x01); 

    /* set gamma */
    lcd_write_index(0xE0);
    lcd_write_data(0x0F);
    lcd_write_data(0x31);
    lcd_write_data(0x2B);
    lcd_write_data(0x0C);
    lcd_write_data(0x0E);
    lcd_write_data(0x08);
    lcd_write_data(0x4E);
    lcd_write_data(0xF1);
    lcd_write_data(0x37);
    lcd_write_data(0x07);
    lcd_write_data(0x10);
    lcd_write_data(0x03);
    lcd_write_data(0x0E);
    lcd_write_data(0x09);
    lcd_write_data(0x00);

    /* set gamma */
    lcd_write_index(0XE1);
    lcd_write_data(0x00);
    lcd_write_data(0x0E);
    lcd_write_data(0x14);
    lcd_write_data(0x03);
    lcd_write_data(0x11);
    lcd_write_data(0x07);
    lcd_write_data(0x31);
    lcd_write_data(0xC1);
    lcd_write_data(0x48);
    lcd_write_data(0x08);
    lcd_write_data(0x0F);
    lcd_write_data(0x0C);
    lcd_write_data(0x31);
    lcd_write_data(0x36);
    lcd_write_data(0x0F);

    /* exit sleep */
    lcd_write_index(0x11);
    delay_1ms(120); 

    /* Enable panel output immediately after init. */
    LCD_CS_SET;
    lcd_display_on();
}

/*!
    \brief      set lcd display region
    \param[in]  x_start: the x position of the start point
    \param[in]  y_start: the y position of the start point
    \param[in]  x_end: the x position of the end point
    \param[in]  y_end: the y position of the end point
    \param[out] none
    \retval     none
*/
void lcd_set_region(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    LCD_CS_CLR;

    /* write the register address 0x2A */
    lcd_write_index(0x2A);
    lcd_write_data_16bit(x_start >> 8,x_start);
    lcd_write_data_16bit(x_end >> 8,x_end);

    /* write the register address 0x2B */
    lcd_write_index(0x2B);
    lcd_write_data_16bit(y_start >> 8,y_start);
    lcd_write_data_16bit(y_end >> 8,y_end);

    /* write the register address 0x2C */
    lcd_write_index(0x2C);
    LCD_CS_SET;
}

/*!
    \brief      set the start display point of lcd
    \param[in]  x: the x position of the start point
    \param[in]  y: the y position of the start point
    \param[out] none
    \retval     none
*/
void lcd_set_xy(uint16_t x, uint16_t y)
{
    /* write the register address 0x2A */
    lcd_write_index(0x2A);
    lcd_write_data_16bit(x >> 8,x);

    /* write the register address 0x2B */
    lcd_write_index(0x2B);
    lcd_write_data_16bit(y >> 8,y);

    /* write the register address 0x2C */
    lcd_write_index(0x2C);
}

/*!
    \brief      draw a point on the lcd
    \param[in]  x: the x position of the point 
    \param[in]  y: the y position of the point 
    \param[in]  data: the register data to be written
    \param[out] none
    \retval     none
*/
void gui_draw_point(uint16_t x,uint16_t y,uint16_t data)
{
    lcd_set_xy(x,y);
    lcd_write_data(data >> 8);
    lcd_write_data(data);
}

/*!
    \brief      send a block of RGB565 pixels continuously (high byte first)
    \param[in]  pixels: pointer to pixel data
    \param[in]  count: number of pixels to send
    \param[out] none
    \retval     none
*/
void lcd_write_pixels(uint16_t *pixels, uint32_t count)
{
    uint32_t i;
    LCD_RS_SET;
    LCD_CS_CLR;
    for(i = 0; i < count; i++) {
        spi_write_byte(pixels[i] >> 8);
        spi_write_byte(pixels[i] & 0xFF);
    }
    LCD_CS_SET;
}

/*! 
    \brief      send a block of bytes continuously by SPI DMA
    \param[in]  bytes: pointer to byte stream
    \param[in]  count: number of bytes to send
    \param[out] none
    \retval     none
*/
void lcd_write_bytes_dma(const uint8_t *bytes, uint32_t count)
{
    uint32_t timeout;
    uint32_t transfer_size;

    if((NULL == bytes) || (0U == count)) {
        return;
    }

    if(0U == lcd_dma_enabled) {
        while(count--) {
            spi_write_byte(*bytes++);
        }
        return;
    }

    while(count > 0U) {
        transfer_size = (count > LCD_DMA_MAX_ONESHOT) ? LCD_DMA_MAX_ONESHOT : count;
        dma_channel_disable(LCD_DMA_TX_CHANNEL);
        dma_flag_clear(LCD_DMA_TX_CHANNEL, DMA_FLAG_FTF | DMA_FLAG_HTF | DMA_FLAG_TAE | DMA_FLAG_SDE | DMA_FLAG_FEE);
        dma_memory_address_config(LCD_DMA_TX_CHANNEL, DMA_MEMORY_0, (uint32_t)bytes);
        dma_transfer_number_config(LCD_DMA_TX_CHANNEL, transfer_size);
        spi_dma_enable(SPI_DMA_TRANSMIT);
        dma_channel_enable(LCD_DMA_TX_CHANNEL);

        timeout = LCD_DMA_TX_TIMEOUT;
        while(RESET == dma_flag_get(LCD_DMA_TX_CHANNEL, DMA_FLAG_FTF)) {
            if(0U == --timeout) {
                dma_channel_disable(LCD_DMA_TX_CHANNEL);
                spi_dma_disable(SPI_DMA_TRANSMIT);
                lcd_dma_enabled = 0U;
                while(count--) {
                    spi_write_byte(*bytes++);
                }
                return;
            }
        }

        dma_channel_disable(LCD_DMA_TX_CHANNEL);
        spi_dma_disable(SPI_DMA_TRANSMIT);
        while(SET == spi_flag_get(SPI_FLAG_TRANS)) {
        }
        lcd_spi_drain_rx();
        dma_flag_clear(LCD_DMA_TX_CHANNEL, DMA_FLAG_FTF | DMA_FLAG_HTF | DMA_FLAG_TAE | DMA_FLAG_SDE | DMA_FLAG_FEE);
        bytes += transfer_size;
        count -= transfer_size;
    }
}

/* ---- Async DMA implementation ---- */

/*!
    \brief      kick off next DMA chunk for async transfer (called from both start and ISR)
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void lcd_dma_kick_next(void)
{
    uint32_t transfer_size;

    transfer_size = (s_async_remaining > LCD_DMA_MAX_ONESHOT) ? LCD_DMA_MAX_ONESHOT : s_async_remaining;
    dma_channel_disable(LCD_DMA_TX_CHANNEL);
    dma_flag_clear(LCD_DMA_TX_CHANNEL, DMA_FLAG_FTF | DMA_FLAG_HTF | DMA_FLAG_TAE | DMA_FLAG_SDE | DMA_FLAG_FEE);
    dma_memory_address_config(LCD_DMA_TX_CHANNEL, DMA_MEMORY_0, (uint32_t)s_async_bytes);
    dma_transfer_number_config(LCD_DMA_TX_CHANNEL, transfer_size);
    spi_dma_enable(SPI_DMA_TRANSMIT);
    dma_channel_enable(LCD_DMA_TX_CHANNEL);
    s_async_bytes += transfer_size;
    s_async_remaining -= transfer_size;
}

void lcd_write_bytes_dma_async(const uint8_t *bytes, uint32_t count,
                               lcd_dma_complete_cb_t cb, void *user_data)
{
    if((NULL == bytes) || (0U == count)) {
        if(cb) {
            cb(user_data);
        }
        return;
    }

    if(0U == lcd_dma_enabled) {
        while(count--) {
            spi_write_byte(*bytes++);
        }
        if(cb) {
            cb(user_data);
        }
        return;
    }

    s_async_cb = cb;
    s_async_user_data = user_data;
    s_async_bytes = bytes;
    s_async_remaining = count;
    s_async_busy = 1U;
    lcd_dma_kick_next();
}

/*!
    \brief      clear the lcd
    \param[in]  color: lcd display color 
    \param[out] none
    \retval     none
*/
void lcd_clear(uint16_t color)
{
    unsigned int i,m;
    /* set lcd display region */
    lcd_set_region(0,0,X_MAX_PIXEL - 1,Y_MAX_PIXEL - 1);
    LCD_RS_SET;

    LCD_CS_CLR;
    for(i = 0;i < Y_MAX_PIXEL;i ++){
        for(m = 0;m < X_MAX_PIXEL;m ++){
            spi_write_byte(color >> 8);
            spi_write_byte(color);
        }
    }
    LCD_CS_SET;
}

/*!
    \brief      DMA full-transfer-complete dispatcher
    \param[in]  none
    \param[out] none
    \retval     none
    \note       Handles async multi-chunk DMA: either kicks off next chunk
                or calls the LVGL flush-ready callback.
*/
static void lcd_dma_ftf_dispatch(void)
{
    if(s_async_busy) {
        dma_channel_disable(LCD_DMA_TX_CHANNEL);
        spi_dma_disable(SPI_DMA_TRANSMIT);
        while(SET == spi_flag_get(SPI_FLAG_TRANS)) {
        }
        lcd_spi_drain_rx();
        dma_flag_clear(LCD_DMA_TX_CHANNEL, DMA_FLAG_FTF | DMA_FLAG_HTF | DMA_FLAG_TAE | DMA_FLAG_SDE | DMA_FLAG_FEE);

        if(s_async_remaining > 0U) {
            lcd_dma_kick_next();
        } else {
            s_async_busy = 0U;
            LCD_CS_SET;
            lcd_dma_complete_cb_t cb = s_async_cb;
            void *ud = s_async_user_data;
            s_async_cb = NULL;
            s_async_user_data = NULL;
            if(cb) {
                cb(ud);
            }
        }
    }
}

void DMA_Channel3_IRQHandler(void) { lcd_dma_ftf_dispatch(); }

void lcd_display_on(void)
{
    LCD_CS_CLR;
    lcd_write_index(0x29);
    LCD_CS_SET;
}

/*! \brief      set region + RS=DATA, CS stays low for DMA pixel stream */
void lcd_flush_region(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    LCD_CS_CLR;
    lcd_write_index(0x2A);
    lcd_write_data_16bit(x_start >> 8, x_start);
    lcd_write_data_16bit(x_end >> 8, x_end);
    lcd_write_index(0x2B);
    lcd_write_data_16bit(y_start >> 8, y_start);
    lcd_write_data_16bit(y_end >> 8, y_end);
    lcd_write_index(0x2C);
    LCD_RS_SET;
    /* CS stays low; DMA ISR will set CS high after pixel transfer */
}
