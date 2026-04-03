#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_clock.h"
#include "nrf_gpio.h"
#include "nrf_ppi.h"
#include "nrf_rtc.h"
#include "nrf_timer.h"
#include "nrf_uart.h"
#include "serial_cmd.h"
#include "uart_printf.h"

#define LED_PIN NRF_PIN_PORT_TO_PIN_NUMBER(15, 0) /* P0.15 */
#define UART_TX_PIN NRF_PIN_PORT_TO_PIN_NUMBER(17, 0) /* P0.17 */
#define UART_RX_PIN NRF_PIN_PORT_TO_PIN_NUMBER(20, 0) /* P0.20 */

static serial_cmd_state_st serial_cmd_state;
static bool enable_sleep = true;

static uint32_t timestamp_offset = 0;

void uart_write_byte(char c) {
  nrf_uart_txd_set(NRF_UART0, c);
}

void uart_tx_start(void) {
  uart_tx_irq_handler();
}

void UART0_IRQHandler(void) {
  uint8_t buf;
  if (nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_TXDRDY)) {
    nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_TXDRDY);
    uart_tx_irq_handler();
  }
  if (nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXDRDY)) {
    buf = nrf_uart_rxd_get(NRF_UART0);
    serial_cmd_push_char(&serial_cmd_state, buf);
    nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXDRDY);
  }
}

void cmd_sleep(void* params) {
  enable_sleep = !enable_sleep;
  uart_printf("enable_sleep: %0d\n", (int)enable_sleep);
}

void cmd_get_rtc(void* params) {
  uint32_t buf;

  nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_CAPTURE0);
  buf = NRF_TIMER0->CC[0] >> 3u;
  uart_printf("RTC: %" PRIu32 "\n", buf + timestamp_offset);
}

void cmd_set_rtc(void* params) {
  char* param_str = (char*)params;

  sscanf(param_str, "%" SCNu32, &timestamp_offset);

  nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_CLEAR);
  nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_CLEAR);
}

void cmd_clr_rtc1(void* params) {
  nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_CLEAR);
  nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_CLEAR);
}

int main(void) {
  char buf;
  bool status_buf;

  nrf_uart_config_t uart_cfg = {.hwfc = NRF_UART_HWFC_DISABLED,
                                .parity = NRF_UART_PARITY_EXCLUDED,
                                .stop = NRF_UART_STOP_TWO};

  nrf_clock_lf_src_set(NRF_CLOCK, NRF_CLOCK_LFCLK_XTAL);

  nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTART);
  while (!nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED));
  nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);

  nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTART);
  while (!nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_LFCLKSTARTED));
  nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_LFCLKSTARTED);

  // RTC1
  nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_STOP);

  nrf_rtc_event_clear(NRF_RTC1, RTC_EVENTS_TICK_EVENTS_TICK_Msk);
  nrf_rtc_event_clear(NRF_RTC1, RTC_EVENTS_COMPARE_EVENTS_COMPARE_Msk);
  nrf_rtc_event_clear(NRF_RTC1, RTC_EVENTS_OVRFLW_EVENTS_OVRFLW_Msk);

  nrf_rtc_prescaler_set(NRF_RTC1, 4095);
  nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_CLEAR);
  nrf_rtc_event_enable(NRF_RTC1, RTC_EVENTS_TICK_EVENTS_TICK_Msk);
  nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_START);

  // TIMER
  nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_STOP);
  nrf_timer_mode_set(NRF_TIMER0, NRF_TIMER_MODE_COUNTER);
  nrf_timer_bit_width_set(NRF_TIMER0, NRF_TIMER_BIT_WIDTH_32);
  nrf_timer_prescaler_set(NRF_TIMER0, 8);
  nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_CLEAR);

  // PPI
  nrf_ppi_channel_disable(NRF_PPI, NRF_PPI_CHANNEL0);
  nrf_ppi_channel_endpoint_setup(
      NRF_PPI, NRF_PPI_CHANNEL0,
      nrf_rtc_event_address_get(NRF_RTC1, NRF_RTC_EVENT_TICK),
      nrf_timer_task_address_get(NRF_TIMER0, NRF_TIMER_TASK_COUNT));
  nrf_ppi_channel_enable(NRF_PPI, NRF_PPI_CHANNEL0);

  nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_START);

  serial_cmd_init(&serial_cmd_state, uart_printf, uart_putchar);

  serial_cmd_add(&serial_cmd_state,
    &(serial_cmd_st){"rtc", "Get RTC value", CMD_NO_ARGS, cmd_get_rtc, NULL});

  serial_cmd_add(&serial_cmd_state,
    &(serial_cmd_st){"set_rtc", "Set RTC value", CMD_STRING, cmd_set_rtc, NULL});

  serial_cmd_add(&serial_cmd_state,
    &(serial_cmd_st){"slp", "Invert enable_sleep", CMD_NO_ARGS, cmd_sleep, NULL});

  nrf_gpio_cfg_output(LED_PIN);

  nrf_gpio_cfg_output(UART_TX_PIN);
  nrf_gpio_pin_clear(UART_TX_PIN);

  nrf_uart_enable(NRF_UART0);

  nrf_uart_txrx_pins_set(NRF_UART0, UART_TX_PIN, UART_RX_PIN);

  nrf_uart_baudrate_set(NRF_UART0, UART_BAUDRATE_BAUDRATE_Baud115200);
  nrf_uart_configure(NRF_UART0, &uart_cfg);

  nrf_uart_int_enable(NRF_UART0, NRF_UART_INT_MASK_TXDRDY);
  nrf_uart_int_enable(NRF_UART0, NRF_UART_INT_MASK_RXDRDY);
  NVIC_EnableIRQ(UART0_IRQn);
  NVIC_SetPriority(UART0_IRQn, 3);

  nrf_uart_task_trigger(NRF_UART0, NRF_UART_TASK_STARTTX);
  nrf_uart_task_trigger(NRF_UART0, NRF_UART_TASK_STARTRX);

  uart_printf("\nTest start\n");

  for (;;) {
    do {
      __disable_irq();
      status_buf = serial_cmd_pop_char(&serial_cmd_state, &buf);
      __enable_irq();
      if (status_buf) {
        serial_cmd_process(&serial_cmd_state, buf);
      }
    } while (status_buf);
    for(volatile int i = 0; i < 10000; i++);
    nrf_gpio_pin_clear(LED_PIN);
    if (enable_sleep) __WFE();
    nrf_gpio_pin_set(LED_PIN);
  }
}
