// file: nxp_uart_dma.h
//
// this driver relies on the kinetis uart_edma driver
// it allows sending bytes without worrying about the size of the buffer used for the dma transfer,
// or whether or not a DMA transfer is in progress.
//
// bytes are buffered in a fifo and sent automatically via the UART DMA transfer complete interrupt
//
// warning: the user must call nxp_uart_xfer_complete_cb in the UART_eDMA_Transfer_Complete callback

#ifndef NXP_UART_DMA_H
#define NXP_UART_DMA_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "fifo.h"
#include "fsl_uart_edma.h"

#ifdef __cplusplus
extern "C" {
#endif

// it's confusing to have to remember the order of a large number of arguments. Using a struct instead.
typedef struct
{
    UART_Type* base;
    uart_edma_handle_t* handle;

    uint8_t* tx_fifo_buf;
    uint16_t tx_fifo_capacity;
    uint8_t* rx_fifo_buf;
	uint16_t rx_fifo_capacity;

    uint8_t* tx_dma_buf;
    uint16_t tx_dma_capacity;

} nxp_uart_dma_init_t;

typedef struct 
{
    UART_Type* base;
    uart_edma_handle_t* handle;

    fifo_t tx_fifo;
    fifo_t rx_fifo;

    uint8_t* tx_dma_buf;
    uint16_t tx_dma_capacity;

    // want to continuously receive, including
    // when the rx_fifo is being read from,
    // which happens in nxp_uart_dma_receive
    //
    // when the transfer complete interrupt occurs,
    // want to switch the dma buffer and call receive again
    // before the next bit comes over the wire.
    uint8_t rx_dma_buf1;
    uint8_t rx_dma_buf2;
    uint8_t next_rx_buf; // 0 --> rx_dma_buf1, 1 --> rx_dma_buf2

    bool tx_in_progress;
    
} nxp_uart_dma_t;

void nxp_uart_dma_init(nxp_uart_dma_t* drv, nxp_uart_dma_init_t* init);

// buffer and send
// returns the number of bytes buffered
uint16_t nxp_uart_dma_send(nxp_uart_dma_t* drv, uint8_t* bytes, uint16_t len);

// the driver automatically receives and buffers bytes.
// this function reads up to `len` bytes from from the buffer, places them into
// `bytes`, and returns the number of bytes read.
//
// if rx_fifo is too big, it's possible to miss an interrupt while calling this function
// for reference: 1/115,200 ~ 8.7x10^(-6).
// for reference: 256 / 100,000,000 ~ 2.6x10^(-6)
uint16_t nxp_uart_dma_receive(nxp_uart_dma_t* drv, uint8_t* bytes, uint16_t len);

// allows easily sending text
uint16_t nxp_uart_dma_printf(nxp_uart_dma_t* drv, const char* fmt, ...);

void nxp_uart_xfer_complete_cb(nxp_uart_dma_t* drv, status_t status);

#ifdef __cplusplus
} // extern "C" { 
#endif

#endif
