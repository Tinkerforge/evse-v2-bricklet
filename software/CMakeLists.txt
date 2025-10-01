CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_config_custom.txt)

SET(PROJECT_NAME evse-v2-bricklet)
SET(CHIP_FAMILY XMC1)
SET(MCU cortex-m0\ -mthumb)
PROJECT(${PROJECT_NAME})

# Optimization level, can be [0, 1, 2, 3, s].
# 0 = turn off optimization. s = optimize for size.
#SET(OPTIMIZATION_LEVEL 0)
SET(DEBUG "-g -ggdb")

SET(OPTIMIZATION_LEVEL s)
#SET(DEBUG "")

SET(CMAKE_BUILD_TYPE None)
ENABLE_LANGUAGE(C ASM)

INCLUDE_DIRECTORIES(
	"${PROJECT_SOURCE_DIR}/src/"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/inc/"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/CMSIS/Include/"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/CMSIS/Infineon/XMC1400_series/Include/"
)

# find source files
SET(SOURCES
	"${PROJECT_SOURCE_DIR}/src/main.c"
	"${PROJECT_SOURCE_DIR}/src/communication.c"
	"${PROJECT_SOURCE_DIR}/src/evse.c"
	"${PROJECT_SOURCE_DIR}/src/adc.c"
	"${PROJECT_SOURCE_DIR}/src/iec61851.c"
	"${PROJECT_SOURCE_DIR}/src/lock.c"
	"${PROJECT_SOURCE_DIR}/src/led.c"
	"${PROJECT_SOURCE_DIR}/src/button.c"
	"${PROJECT_SOURCE_DIR}/src/dc_fault.c"
	"${PROJECT_SOURCE_DIR}/src/charging_slot.c"
	"${PROJECT_SOURCE_DIR}/src/hardware_version.c"
	"${PROJECT_SOURCE_DIR}/src/phase_control.c"
	"${PROJECT_SOURCE_DIR}/src/tmp1075n.c"
	"${PROJECT_SOURCE_DIR}/src/eichrecht.c"

	"${PROJECT_SOURCE_DIR}/src/bricklib2/warp/contactor_check.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/warp/rs485.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/warp/modbus.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/warp/meter.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/warp/meter_eltako.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/warp/meter_eastron.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/warp/meter_iskra.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/warp/meter_generic.c"

	"${PROJECT_SOURCE_DIR}/src/bricklib2/hal/uartbb/uartbb.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/hal/system_timer/system_timer.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/hal/startup/startup_xmc1.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/hal/startup/startup_xmc1400.S"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/hal/spi_fifo/spi_fifo.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/hal/i2c_fifo/i2c_fifo.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/hal/ccu4_pwm/ccu4_pwm.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/os/coop_task.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/os/coop_task_pendsv_handler.S"

	"${PROJECT_SOURCE_DIR}/src/bricklib2/bootloader/bootloader.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/bootloader/bootloader_xmc_irq.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/protocols/tfp/tfp.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/logging/logging.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/utility/ringbuffer.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/utility/pearson_hash.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/utility/communication_callback.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/utility/led_flicker.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/utility/moving_average.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/utility/ringbuffer.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/utility/crc16.c"

	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc_gpio.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc1_gpio.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc_spi.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc_usic.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc1_scu.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc1_flash.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc_ccu4.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc_ccu8.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc_vadc.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc1_scu.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc_uart.c"
	"${PROJECT_SOURCE_DIR}/src/bricklib2/xmclib/XMCLib/src/xmc_i2c.c"
)

MESSAGE(STATUS "\nFound following source files:\n ${SOURCES}\n")

# define executable
include(${CMAKE_CURRENT_SOURCE_DIR}/src/bricklib2/cmake/configs/config_define_executable.txt)

# add aeabi aeabi_idiv, aeabi_uidiv, aeabi_idivmod and aeabi_uidivmod
include(${CMAKE_CURRENT_SOURCE_DIR}/src/bricklib2/cmake/configs/config_comcu_add_aeabi_libraries.txt)

# define standard compile and linker flags
SET(LINKER_SCRIPT_NAME xmc1_firmware_with_brickletboot.ld)
SET(FLASH_ORIGIN 0x10003000) # Move flash origin above the bootloader
SET(FLASH_EEPROM_LENGTH 1024) # Flash used for EEPROM emulation at end of flash (multiple of page size (256 byte))
MATH(EXPR FLASH_LENGTH "${CHIP_FLASH_SIZE} - 8192 - ${FLASH_EEPROM_LENGTH}") # Remove bootloader size from flash size
include(${CMAKE_CURRENT_SOURCE_DIR}/src/bricklib2/cmake/configs/config_comcu_add_standard_flags.txt)

# add custom build commands
include(${CMAKE_CURRENT_SOURCE_DIR}/src/bricklib2/cmake/configs/config_comcu_add_standard_custom_commands.txt)

# add preprocessor defines
include(${CMAKE_CURRENT_SOURCE_DIR}/src/bricklib2/cmake/configs/config_xmc1_add_preprocessor_defines.txt)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}.elf -lm) # Use lib math (sqrt etc)
ADD_DEFINITIONS(-DARM_MATH_CM0) # Use CMSIS DSP math support
ADD_DEFINITIONS(-D__ARM_FEATURE_DSP=0) # Cortex-M0 doesn't have DSP instructions

# Make sure constants are single precision by default
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsingle-precision-constant")

# We already define -Wall in config_comcu_add_standard_flags.txt
# Add extra warnings
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")

# Additional warnings (beyond -Wextra)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdouble-promotion")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfloat-conversion")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wduplicated-cond")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wduplicated-branches")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wnull-dereference")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wjump-misses-init")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstack-usage=1024")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wundef")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshift-overflow=2")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-conversion")

# Treat warnings as errors in EVSE firmware
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")

# Generate linker map in build/ folder for size analysis
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-Map=statistics.map")

# LTO + Os will reduce flash size by ~10kb
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")