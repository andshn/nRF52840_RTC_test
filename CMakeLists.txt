cmake_minimum_required(VERSION 3.17)
project(NRF_TEST_RTC C ASM)
set(FIRMWARE_TARGET rtc_test)

find_package(Python REQUIRED)

set(CMAKE_SYSTEM_NAME Generic)

if(WIN32)
  set(TOOLCHAIN_BIN_SUFFIX ".exe")
  set(TOOLCHAIN_PATH "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/")
else()
  set(TOOLCHAIN_BIN_SUFFIX "")
  set(TOOLCHAIN_PATH "/usr/bin/")
endif()

set(TOOLCHAIN_PREFIX "${TOOLCHAIN_PATH}arm-none-eabi-")

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc${TOOLCHAIN_BIN_SUFFIX})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++${TOOLCHAIN_BIN_SUFFIX})
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc${TOOLCHAIN_BIN_SUFFIX})
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy${TOOLCHAIN_BIN_SUFFIX})
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size${TOOLCHAIN_BIN_SUFFIX})

set(NRFX_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/nrfx)
set(CMSIS_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/CMSIS_6/CMSIS/)
set(LD_FILE ${CMAKE_CURRENT_LIST_DIR}/linker/nrf52840_s140_v6.ld)
set(UF2CONV_PY ${CMAKE_CURRENT_LIST_DIR}/lib/uf2/utils/uf2conv.py)
set(NRF_FAMILY 0xADA52840)

set(CMAKE_EXECUTABLE_SUFFIX .elf)

add_executable(${FIRMWARE_TARGET}
  src/main.c
  src/uart_printf.c
  src/serial_cmd.c
  ${NRFX_DIR}/bsp/stable/mdk/gcc_startup_nrf52840.S
  ${NRFX_DIR}/bsp/stable/mdk/system_nrf52.c
)

target_compile_definitions(${FIRMWARE_TARGET} PUBLIC NRF52840_XXAA)

target_include_directories(${FIRMWARE_TARGET} PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/inc/
  ${NRFX_DIR}
  ${NRFX_DIR}/bsp/stable/mdk/
  ${NRFX_DIR}/bsp/stable/
  ${NRFX_DIR}/hal/
  ${CMSIS_DIR}/Core/Include/
)

target_link_options(${FIRMWARE_TARGET} PUBLIC
  -L${NRFX_DIR}/bsp/stable/mdk/
  "LINKER:--script=${LD_FILE}"
  -mcpu=cortex-m4
  -nostartfiles
  --specs=nosys.specs
  --specs=nano.specs
  -u _printf_float
  -Wl,--gc-sections
  -Wl,--undefined=__start
)

target_compile_options(${FIRMWARE_TARGET} PUBLIC
  -std=c99

  -mthumb
  -mcpu=cortex-m4
  -mfloat-abi=soft
  -mfpu=fpv4-sp-d16

  -fno-builtin
  -fshort-enums
  -fstack-usage
  -fno-strict-aliasing
  -Wall
  -Wextra
  -Wno-unused-parameter -Wno-expansion-to-defined -Wno-array-bounds
)
  
target_compile_definitions(${FIRMWARE_TARGET} PUBLIC
  SOFTDEVICE_PRESENT
  CONFIG_GPIO_AS_PINRESET
  __START=main
  __STARTUP_CLEAR_BSS
)

add_custom_command(TARGET ${FIRMWARE_TARGET} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${FIRMWARE_TARGET}> $<TARGET_FILE_DIR:${FIRMWARE_TARGET}>/${FIRMWARE_TARGET}.hex
  COMMAND ${Python_EXECUTABLE} ${UF2CONV_PY} ${FIRMWARE_TARGET}.hex -c -f ${NRF_FAMILY} -o ${FIRMWARE_TARGET}.uf2
)
