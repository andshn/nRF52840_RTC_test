#ifndef __SERIAL_CMD_H__
#define __SERIAL_CMD_H__

#include <stdint.h>
#include <stdbool.h>

#define SERIAL_CMD_FIFO_SIZE 512
#define SERIAL_CMD_BUF_SIZE 512

typedef enum {
  CMD_NO_ARGS,
  CMD_ONE_INT,
  CMD_TWO_INT,
  CMD_STRING
} serial_cmd_arg_type_st;

typedef struct serial_cmd {
  const char* name;
  const char* description;
  serial_cmd_arg_type_st arg_type;
  void (*handler)(void* params);
  struct serial_cmd* next;
} serial_cmd_st;

typedef struct {
  volatile char buffer[SERIAL_CMD_FIFO_SIZE];
  volatile uint16_t head;
  volatile uint16_t tail;
  volatile bool overflow;
} serial_cmd_fifo_st;

typedef struct {
  serial_cmd_st* cmd_list;
  serial_cmd_fifo_st fifo;
  char line_buffer[SERIAL_CMD_BUF_SIZE];
  uint16_t line_index;
  void (*out_printf_func)(const char* format, ...);
  void (*out_putc_func)(char);
} serial_cmd_state_st;

void serial_cmd_init(serial_cmd_state_st* state,
                     void (*out_printf_func)(const char*, ...),
                     void (*out_putc_func)(char));

void serial_cmd_add(serial_cmd_state_st* state, serial_cmd_st* cmd);
void serial_cmd_push_char(serial_cmd_state_st* state, char c); // Вызывается в прерывании.
bool serial_cmd_pop_char(serial_cmd_state_st* state, char* c); // Вызывается в основном цикле в критической секции.
void serial_cmd_process(serial_cmd_state_st* state, char c); // Вызывается в основном цикле.

#endif
