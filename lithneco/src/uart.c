/**
 * \file
 *
 * \brief UART functions for AVR Xmega
 *
 * Copyright (c) 2011 - 2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include <asf.h>
#include "conf_board.h"
#include "uart.h"
#include "main.h"
#include "ui.h"

void uart_config(usartSpecification usartSpec, usb_cdc_line_coding_t * cfg)
{
	uint8_t reg_ctrlc;
	uint16_t bsel;

	reg_ctrlc = USART_CMODE_ASYNCHRONOUS_gc;

	switch (cfg->bCharFormat) {
	case CDC_STOP_BITS_2:
		reg_ctrlc |= USART_SBMODE_bm;
		break;
	case CDC_STOP_BITS_1:
	default:
		// Default stop bit = 1 stop bit
		break;
	}

	switch (cfg->bParityType) {
	case CDC_PAR_EVEN:
		reg_ctrlc |= USART_PMODE_EVEN_gc;
		break;
	case CDC_PAR_ODD:
		reg_ctrlc |= USART_PMODE_ODD_gc;
		break;
	default:
	case CDC_PAR_NONE:
		reg_ctrlc |= USART_PMODE_DISABLED_gc;
		break;
	}

	switch (cfg->bDataBits) {
	case 5:
		reg_ctrlc |= USART_CHSIZE_5BIT_gc;
		break;
	case 6:
		reg_ctrlc |= USART_CHSIZE_6BIT_gc;
		break;
	case 7:
		reg_ctrlc |= USART_CHSIZE_7BIT_gc;
		break;
	default:
	case 8:
		reg_ctrlc |= USART_CHSIZE_8BIT_gc;
		break;
	case 9:
		reg_ctrlc |= USART_CHSIZE_9BIT_gc;
		break;
	}

	sysclk_enable_module(usartSpec.sysClkPort, usartSpec.sysClk);
	// Set configuration
	usartSpec.usartPtr->CTRLC = reg_ctrlc;
	// Update baudrate
	bsel = (uint16_t) (((((((uint32_t) sysclk_get_cpu_hz()) << 1) / ((uint32_t)
		le32_to_cpu(cfg->dwDTERate) * 8)) + 1) >> 1) - 1);
	usartSpec.usartPtr->BAUDCTRLA = bsel & 0xFF;
	usartSpec.usartPtr->BAUDCTRLB = bsel >> 8;
}

void uart_open(usartSpecification usartSpec)
{
	sysclk_enable_module(usartSpec.sysClkPort, usartSpec.sysClk);
	// Open UART communication
	usartSpec.portPtr->DIRSET = usartSpec.pinTx; // TX as output.
	usartSpec.portPtr->DIRCLR = usartSpec.pinRx; // RX as input.

	// Enable both RX and TX
	usartSpec.usartPtr->CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_CLK2X_bm;
	// Enable interrupt with priority higher than USB
	usartSpec.usartPtr->CTRLA = (register8_t) USART_RXCINTLVL_HI_gc | (register8_t)
			USART_DREINTLVL_OFF_gc;
}

void uart_close(usartSpecification usartSpec)
{
	sysclk_disable_module(usartSpec.sysClkPort, usartSpec.sysClk);
	// Disable interrupts
	usartSpec.usartPtr->CTRLA = 0;
	// Close RS232 communication
	usartSpec.usartPtr->CTRLB = 0;
}

void uart_rx_notify(usartSpecification usartSpec)
{
	// If UART is open
	if (usartSpec.usartPtr->CTRLA != 0) {
		// Enable UART TX interrupt to send values
		usartSpec.usartPtr->CTRLA = (register8_t) USART_RXCINTLVL_HI_gc | (register8_t)
		USART_DREINTLVL_HI_gc;
	}
}

// Interrupt vectors for COMM0
ISR(USART_COMM0_RX_Vect)
{
	uint8_t value;

	// Data received
	ui_com_tx_start();

	if (0 != (USART_COMM0.STATUS & (USART_FERR_bm | USART_BUFOVF_bm))) {
		udi_cdc_multi_signal_framing_error(0);
		ui_com_error();
	}

	// Transfer UART RX fifo to CDC TX
	value = USART_COMM0.DATA;
	if (!udi_cdc_multi_is_tx_ready(0)) {
		// Fifo full
		udi_cdc_multi_signal_overrun(0);
		ui_com_overflow();
	}else{
		udi_cdc_multi_putc(0, value);
	}
	ui_com_tx_stop();
}

ISR(USART_COMM0_DRE_Vect)
{
	// Data send
	if (udi_cdc_multi_is_rx_ready(0)) {
		// Transmit next data
		ui_com_rx_start();
		USART_COMM0.DATA = udi_cdc_multi_getc(0);
	} else {
		// Fifo empty then Stop UART transmission
		USART_COMM0.CTRLA = (register8_t) USART_RXCINTLVL_HI_gc |
				(register8_t) USART_DREINTLVL_OFF_gc;
		ui_com_rx_stop();
	}
}

// Interrupt vectors for COMM1
ISR(USART_COMM1_RX_Vect)
{
	uint8_t value;

	// Data received
	ui_com_tx_start();

	if (0 != (USART_COMM1.STATUS & (USART_FERR_bm | USART_BUFOVF_bm))) {
		udi_cdc_multi_signal_framing_error(1);
		ui_com_error();
	}

	// Transfer UART RX fifo to CDC TX
	value = USART_COMM1.DATA;
	if (!udi_cdc_multi_is_tx_ready(1)) {
		// Fifo full
		udi_cdc_multi_signal_overrun(1);
		ui_com_overflow();
		}else{
		udi_cdc_multi_putc(1, value);
	}
	ui_com_tx_stop();
}

ISR(USART_COMM1_DRE_Vect)
{
	// Data send
	if (udi_cdc_multi_is_rx_ready(1)) {
		// Transmit next data
		ui_com_rx_start();
		USART_COMM1.DATA = udi_cdc_multi_getc(1);
		} else {
		// Fifo empty then Stop UART transmission
		USART_COMM1.CTRLA = (register8_t) USART_RXCINTLVL_HI_gc |
		(register8_t) USART_DREINTLVL_OFF_gc;
		ui_com_rx_stop();
	}
}

// Interrupt vectors for COMM1
ISR(USART_XBEE_RX_Vect)
{
	uint8_t value;

	// Data received
	ui_com_tx_start();

	if (0 != (USART_XBEE.STATUS & (USART_FERR_bm | USART_BUFOVF_bm))) {
		udi_cdc_multi_signal_framing_error(2);
		ui_com_error();
	}

	// Transfer UART RX fifo to CDC TX
	value = USART_XBEE.DATA;
	if (!udi_cdc_multi_is_tx_ready(2)) {
		// Fifo full
		udi_cdc_multi_signal_overrun(2);
		ui_com_overflow();
		}else{
		udi_cdc_multi_putc(2, value);
	}
	ui_com_tx_stop();
}

ISR(USART_XBEE_DRE_Vect)
{
	// Data send
	if (udi_cdc_multi_is_rx_ready(2)) {
		// Transmit next data
		ui_com_rx_start();
		USART_XBEE.DATA = udi_cdc_multi_getc(2);
		} else {
		// Fifo empty then Stop UART transmission
		USART_XBEE.CTRLA = (register8_t) USART_RXCINTLVL_HI_gc |
		(register8_t) USART_DREINTLVL_OFF_gc;
		ui_com_rx_stop();
	}
}