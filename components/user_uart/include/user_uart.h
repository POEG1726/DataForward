/*
    user_uart.h
    Created on Feb 21, 2025
    Author: @POEG1726
*/

#ifndef _USER_UART_H_
#define _USER_UART_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void Init_uart(void);
void uart_send(const char* msg, uint16_t msg_len);

extern QueueHandle_t uart_queue;

#endif // _USER_UART_H_