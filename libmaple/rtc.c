/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @file rtc.c
 * @author Sean Cross <xobs@kosagi.com>
 * @brief Real Time Clock (RTC) support.
 */

#include "rtc.h"
#include "rcc.h"
#include "bkp.h"
#include "usart.h"

/*
 * RTC device
 */

static rtc_dev rtc = {
    .regs     = RTC_BASE,
    .clk_id   = 0,
    .irq_num  = NVIC_RTC,
};
rtc_dev *RTC = &rtc;


/*
 * RTC convenience routines
 */

/**
 * @brief Initialize the RTC
 * @param dev Device to initialize and reset.
 */
void rtc_init(rtc_dev *dev) {
    //RCC_BASE->BDCR &= RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_LSE;
    RCC_BASE->APB1ENR |= RCC_APB1ENR_PWREN;

    /* Disable Backup Write Protection so we can modify RTC bits */
    bkp_enable_writes();

    RCC_BASE->BDCR |= RCC_BDCR_LSEON;
    usart_putc(USART1, 'a');
    while (!(RCC_BASE->BDCR & RCC_BDCR_LSERDY));

    /* Enable the RTC */
    RCC_BASE->BDCR |= RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_LSE;

    /* Wait for it to stabilize and setup RTC for config */
    while(!(dev->regs->CRL & RTC_CRL_RTOFF));
    dev->regs->CRL |= RTC_CRL_CNF;

    /* Set the 32 kHz oscillator to have a divider of 32768 */
    dev->regs->PRLH = 0x0;
    dev->regs->PRLL = 0x7fff;

    /* Flush the config to the analog domain and wait for sync */
    dev->regs->CRL &= ~RTC_CRL_CNF;
    while(!(dev->regs->CRL & RTC_CRL_RTOFF));

    /* Disable writes to the backup region */
    bkp_disable_writes();
}


void rtc_set_time(rtc_dev *dev, uint32 time) {
    bkp_enable_writes();

    /* Wait for RTC writes to stabilize */
    while(!(dev->regs->CRL & RTC_CRL_RTOFF));
    dev->regs->CRL |= RTC_CRL_CNF;

    dev->regs->CNTH = (time >> 16);
    dev->regs->CNTL = (time & 0xffff);

    /* Flush the config to the analog domain and wait for sync */
    dev->regs->CRL &= ~RTC_CRL_CNF;
    while(!(dev->regs->CRL & RTC_CRL_RTOFF));

    /* Disable writes to the backup region */
    bkp_disable_writes();
}

uint32 rtc_get_time(rtc_dev *dev) {
    return ((dev->regs->CNTH << 16) & 0xffff0000) | ((dev->regs->CNTL & 0xffff));
}



#if 0
/**
 * @brief Configure GPIO bit modes for use as a SPI port's pins.
 * @param as_master If true, configure bits for use as a bus master.
 *                  Otherwise, configure bits for use as slave.
 * @param nss_dev NSS pin's GPIO device
 * @param comm_dev SCK, MISO, MOSI pins' GPIO device
 * @param nss_bit NSS pin's GPIO bit on nss_dev
 * @param sck_bit SCK pin's GPIO bit on comm_dev
 * @param miso_bit MISO pin's GPIO bit on comm_dev
 * @param mosi_bit MOSI pin's GPIO bit on comm_dev
 */
void spi_gpio_cfg(uint8 as_master,
                  gpio_dev *nss_dev,
                  uint8 nss_bit,
                  gpio_dev *comm_dev,
                  uint8 sck_bit,
                  uint8 miso_bit,
                  uint8 mosi_bit) {
    if (as_master) {
        gpio_set_mode(comm_dev, sck_bit, GPIO_AF_OUTPUT_PP);
        gpio_set_mode(comm_dev, mosi_bit, GPIO_AF_OUTPUT_PP);
#if !defined(BOARD_safecast)
        gpio_set_mode(nss_dev, nss_bit, GPIO_AF_OUTPUT_PP);
        gpio_set_mode(comm_dev, miso_bit, GPIO_INPUT_FLOATING);
#endif
    } else {
        gpio_set_mode(comm_dev, sck_bit, GPIO_INPUT_FLOATING);
        gpio_set_mode(comm_dev, miso_bit, GPIO_AF_OUTPUT_PP);
#if !defined(BOARD_safecast)
        gpio_set_mode(nss_dev, nss_bit, GPIO_INPUT_FLOATING);
        gpio_set_mode(comm_dev, mosi_bit, GPIO_INPUT_FLOATING);
#endif
    }
}

/**
 * @brief Configure and enable a SPI device as bus master.
 *
 * The device's peripheral will be disabled before being reconfigured.
 *
 * @param dev Device to configure as bus master
 * @param baud Bus baud rate
 * @param mode SPI mode
 * @param flags Logical OR of spi_cfg_flag values.
 * @see spi_cfg_flag
 */
void spi_master_enable(spi_dev *dev,
                       spi_baud_rate baud,
                       spi_mode mode,
                       uint32 flags) {
    spi_reconfigure(dev, baud | flags | SPI_CR1_MSTR | mode);
}

/**
 * @brief Configure and enable a SPI device as a bus slave.
 *
 * The device's peripheral will be disabled before being reconfigured.
 *
 * @param dev Device to configure as a bus slave
 * @param mode SPI mode
 * @param flags Logical OR of spi_cfg_flag values.
 * @see spi_cfg_flag
 */
void spi_slave_enable(spi_dev *dev, spi_mode mode, uint32 flags) {
    spi_reconfigure(dev, flags | mode);
}

/**
 * @brief Nonblocking SPI transmit.
 * @param dev SPI port to use for transmission
 * @param buf Buffer to transmit.  The sizeof buf's elements are
 *            inferred from dev's data frame format (i.e., are
 *            correctly treated as 8-bit or 16-bit quantities).
 * @param len Maximum number of elements to transmit.
 * @return Number of elements transmitted.
 */
uint32 spi_tx(spi_dev *dev, const void *buf, uint32 len) {
    uint32 txed = 0;
    uint8 byte_frame = spi_dff(dev) == SPI_DFF_8_BIT;
    while (spi_is_tx_empty(dev) && (txed < len)) {
        if (byte_frame) {
            dev->regs->DR = ((const uint8*)buf)[txed++];
        } else {
            dev->regs->DR = ((const uint16*)buf)[txed++];
        }
    }
    return txed;
}

/**
 * @brief Call a function on each SPI port
 * @param fn Function to call.
 */
void spi_foreach(void (*fn)(spi_dev*)) {
    fn(SPI1);
    fn(SPI2);
#ifdef STM32_HIGH_DENSITY
    fn(SPI3);
#endif
}

/**
 * @brief Enable a SPI peripheral
 * @param dev Device to enable
 */
void spi_peripheral_enable(spi_dev *dev) {
    bb_peri_set_bit(&dev->regs->CR1, SPI_CR1_SPE_BIT, 1);
}

/**
 * @brief Disable a SPI peripheral
 * @param dev Device to disable
 */
void spi_peripheral_disable(spi_dev *dev) {
    bb_peri_set_bit(&dev->regs->CR1, SPI_CR1_SPE_BIT, 0);
}

/**
 * @brief Enable DMA requests whenever the transmit buffer is empty
 * @param dev SPI device on which to enable TX DMA requests
 */
void spi_tx_dma_enable(spi_dev *dev) {
    bb_peri_set_bit(&dev->regs->CR2, SPI_CR2_TXDMAEN_BIT, 1);
}

/**
 * @brief Disable DMA requests whenever the transmit buffer is empty
 * @param dev SPI device on which to disable TX DMA requests
 */
void spi_tx_dma_disable(spi_dev *dev) {
    bb_peri_set_bit(&dev->regs->CR2, SPI_CR2_TXDMAEN_BIT, 0);
}

/**
 * @brief Enable DMA requests whenever the receive buffer is empty
 * @param dev SPI device on which to enable RX DMA requests
 */
void spi_rx_dma_enable(spi_dev *dev) {
    bb_peri_set_bit(&dev->regs->CR2, SPI_CR2_RXDMAEN_BIT, 1);
}

/**
 * @brief Disable DMA requests whenever the receive buffer is empty
 * @param dev SPI device on which to disable RX DMA requests
 */
void spi_rx_dma_disable(spi_dev *dev) {
    bb_peri_set_bit(&dev->regs->CR2, SPI_CR2_RXDMAEN_BIT, 0);
}

/*
 * SPI auxiliary routines
 */

static void spi_reconfigure(spi_dev *dev, uint32 cr1_config) {
    spi_irq_disable(dev, SPI_INTERRUPTS_ALL);
    spi_peripheral_disable(dev);
    dev->regs->CR1 = cr1_config;
    spi_peripheral_enable(dev);
}

/*
 * IRQ handlers (TODO)
 */
#endif
