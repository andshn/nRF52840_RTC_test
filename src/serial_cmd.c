#include "serial_cmd.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

void serial_cmd_push_char(serial_cmd_state_st* state, char c) {
  uint16_t next_head = (state->fifo.head + 1) % SERIAL_CMD_FIFO_SIZE;

  if (next_head == state->fifo.tail) {
    state->fifo.overflow = true;
  } else {
    state->fifo.buffer[state->fifo.head] = c;
    state->fifo.head = next_head;
  }
}

bool serial_cmd_pop_char(serial_cmd_state_st* state, char* c) {
  if (state->fifo.head == state->fifo.tail) {
    return false;
  }

  *c = state->fifo.buffer[state->fifo.tail];
  state->fifo.tail = (state->fifo.tail + 1) % SERIAL_CMD_FIFO_SIZE;
  return true;
}

void serial_cmd_init(serial_cmd_state_st* state,
                     void (*out_printf_func)(const char*, ...),
                     void (*out_putc_func)(char)) {
  state->cmd_list = NULL;
  state->fifo.head = 0;
  state->fifo.tail = 0;
  state->fifo.overflow = false;
  state->line_index = 0;
  state->out_printf_func = out_printf_func;
  state->out_putc_func = out_putc_func;
}

void serial_cmd_add(serial_cmd_state_st* state, serial_cmd_st* cmd) {
  cmd->next = state->cmd_list;
  state->cmd_list = cmd;
}

bool serial_cmd_process_char(serial_cmd_state_st* state, char c) {
  if (state->out_putc_func) state->out_putc_func(c);

  if (c == '\r' || c == '\n') {
    if (state->line_index > 0) {
      state->line_buffer[state->line_index] = '\0';
      state->line_index = 0;

      if (state->out_putc_func) {
        state->out_putc_func('\r');
        state->out_putc_func('\n');
      }
      return true;
    }
    if (state->out_putc_func) {
      state->out_putc_func('\r');
      state->out_putc_func('\n');
    }
    return false;
  }

  if (c == '\b' || c == 0x7F) {
    if (state->line_index > 0) {
      state->line_index--;
      if (state->out_putc_func) {
        state->out_putc_func('\b');
        state->out_putc_func(' ');
        state->out_putc_func('\b');
      }
    }
    return false;
  }

  if (state->line_index < SERIAL_CMD_BUF_SIZE - 1) {
    state->line_buffer[state->line_index++] = c;
  } else {
    state->line_index = 0;
    if (state->out_printf_func) {
      state->out_printf_func("\nERROR: Line buffer overflow\n");
    }
  }

  return false;
}

static void serial_run_command(serial_cmd_state_st* state, serial_cmd_st* cmd,
                               char* args) {
  bool has_args = (*args != '\0');

  switch (cmd->arg_type) {
    case CMD_NO_ARGS:
      if (has_args) {
        if (state->out_printf_func) {
          state->out_printf_func(
              "Error: Command '%s' does not take arguments\r\n", cmd->name);
        }
        return;
      }
      if (cmd->handler) cmd->handler(NULL);
      break;

    case CMD_ONE_INT:
      if (!has_args) {
        if (state->out_printf_func) {
          state->out_printf_func(
              "Error: Command '%s' requires one integer argument\r\n",
              cmd->name);
        }
        return;
      }
      {
        int32_t val;
        sscanf(args, "%" SCNd32, &val);
        if (cmd->handler) cmd->handler(&val);
      }
      break;

    case CMD_TWO_INT:
      if (!has_args) {
        if (state->out_printf_func) {
          state->out_printf_func(
              "Error: Command '%s' requires two integer arguments\r\n",
              cmd->name);
        }
        return;
      }
      {
        int32_t val[2];
        sscanf(args, "%" SCNd32 " " "%" SCNd32, &val[0], &val[1]);
        if (cmd->handler) cmd->handler(val);
      }
      break;
    case CMD_STRING:
      if (cmd->handler) cmd->handler(args);
      break;
  }
}

void serial_cmd_process_line(serial_cmd_state_st* state) {
  char* cmd_name;
  char* cmd_args;
  serial_cmd_st* cmd;
  char* line = state->line_buffer;

  while (*line == ' ' || *line == '\t') {
    line++;
  }

  cmd_name = line;
  cmd_args = line;

  while (*cmd_args && *cmd_args != ' ' && *cmd_args != '\t') {
    cmd_args++;
  }

  if (*cmd_args) {
    *cmd_args = '\0';
    cmd_args++;
    while (*cmd_args == ' ' || *cmd_args == '\t') {
      cmd_args++;
    }
  }

  cmd = state->cmd_list;
  while (cmd) {
    if (strcmp(cmd->name, cmd_name) == 0) {
      serial_run_command(state, cmd, cmd_args);
      return;
    }
    cmd = cmd->next;
  }
  state->out_printf_func("Unknown command: %s\n", cmd_name);
}

void serial_cmd_process(serial_cmd_state_st* state, char c) {
  if (state->fifo.overflow) {
    state->fifo.head = 0;
    state->fifo.tail = 0;
    state->fifo.overflow = false;
    state->line_index = 0;
    state->out_printf_func("\nERROR: RX FIFO overflow\n");
  }
    if (serial_cmd_process_char(state, c)) {
      serial_cmd_process_line(state);
    }
}
