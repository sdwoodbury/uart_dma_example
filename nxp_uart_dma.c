// file: nxp_uart_dma.c

#include "nxp_uart_dma.h"

#include <stdio.h>
#include <string.h>

static void _send(nxp_uart_dma_t* drv);
static void _receive(nxp_uart_dma_t* drv);

void nxp_uart_dma_init(nxp_uart_dma_t* drv, nxp_uart_dma_init_t* init)
{
    memset(drv, 0, sizeof(nxp_uart_dma_t));
    drv->base = init->base;
    drv->handle = init->handle;

    fifo_init(&drv->tx_fifo, init->tx_fifo_buf, init->tx_fifo_capacity, 1);
    fifo_init(&drv->rx_fifo, init->rx_fifo_buf, init->rx_fifo_capacity, 1);

    drv->tx_dma_buf = init->tx_dma_buf;
    drv->tx_dma_capacity = init->tx_dma_capacity;

	// begin reception
	_receive(drv);
}

uint16_t nxp_uart_dma_send(nxp_uart_dma_t* drv, uint8_t* bytes, uint16_t len)
{
	// disable irq for modifying tx_fifo and tx_in_progress
	/////////////////////////////////////////////////////////
    __disable_irq();
    /////////////////////////////////////////////////////////

    uint16_t written = fifo_enqueue(&drv->tx_fifo, bytes, len);
    if (!drv->tx_in_progress)
    {
    	drv->tx_in_progress = true;
    	//////////////////////////////////////////////////////
    	__enable_irq();
    	/////////////////////////////////////////////////////

    	// if tx_in_progress == false then don't have to worry about being
    	// interrupted by nxp_uart_xfer_complete_cb.
    	_send(drv);
    }
    else
    {
    	//////////////////////////////////////////////////////
    	__enable_irq();
    	/////////////////////////////////////////////////////
    }
    assert(len == written);
    return written;
}

uint16_t nxp_uart_dma_receive(nxp_uart_dma_t* drv, uint8_t* bytes, uint16_t len)
{
	// disable irq for modifying rx_fifo
	/////////////////////////////////////////////////////////
	__disable_irq();
	/////////////////////////////////////////////////////////

	uint16_t num_read = fifo_dequeue(&drv->rx_fifo, bytes, len);

	//////////////////////////////////////////////////////
	__enable_irq();
	/////////////////////////////////////////////////////

	return num_read;
}

uint16_t nxp_uart_dma_printf(nxp_uart_dma_t* drv, const char* fmt, ...)
{
	char buffer[256] = {0};
	va_list args;
	va_start (args, fmt);
	vsnprintf (buffer, sizeof(buffer), fmt, args);
	va_end (args);

	uint16_t len = strlen(buffer);
	return nxp_uart_dma_send(drv, (uint8_t*)buffer, len);
}

// called from interrupt context. automatically continue the transfer until empty
void nxp_uart_xfer_complete_cb(nxp_uart_dma_t* drv, status_t status)
{
	// tx finished
	if (kStatus_UART_TxIdle == status)
	{
		_send(drv);
	}
	else if (kStatus_UART_RxIdle == status)
	{
		// this is admittedly a little confusing. next_rx_buf is
		// used to determine which buffer received the byte.
		// the operation is the opposite of the one performed in _receive
	uint8_t* cur_buf = (drv->next_rx_buf == 0) ? &drv->rx_dma_buf2 : &drv->rx_dma_buf1;

		_receive(drv);
		fifo_enqueue(&drv->rx_fifo, cur_buf, 1);
	}
}

static void _send(nxp_uart_dma_t* drv)
{
   uint16_t to_send = fifo_dequeue(&drv->tx_fifo, drv->tx_dma_buf, drv->tx_dma_capacity);

    if (to_send == 0)
    {
        drv->tx_in_progress = false;
        return;
    }

    uart_transfer_t transfer = {
        .data = drv->tx_dma_buf,
        .dataSize = to_send,
    };

    status_t status = UART_SendEDMA(drv->base, drv->handle, &transfer);
    assert(status != kStatus_UART_TxBusy);
}

static void _receive(nxp_uart_dma_t* drv)
{
   uint8_t* next_buf = (drv->next_rx_buf == 0) ? &drv->rx_dma_buf1 : &drv->rx_dma_buf2;
	drv->next_rx_buf ^= 1; // toggle

	uart_transfer_t transfer = {
		.data = next_buf,
		.dataSize = 1,
	};

	status_t status = UART_ReceiveEDMA(drv->base, drv->handle, &transfer);
	assert(status != kStatus_UART_RxBusy);
}
