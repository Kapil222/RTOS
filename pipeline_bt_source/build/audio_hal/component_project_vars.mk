# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += /home/kapil/adf_kapil/esp-adf/components/audio_hal/include /home/kapil/adf_kapil/esp-adf/components/audio_hal/driver/es8388 /home/kapil/adf_kapil/esp-adf/components/audio_hal/driver/es8374 /home/kapil/adf_kapil/esp-adf/components/audio_hal/driver/es8311 /home/kapil/adf_kapil/esp-adf/components/audio_hal/driver/es7243 /home/kapil/adf_kapil/esp-adf/components/audio_hal/driver/zl38063 /home/kapil/adf_kapil/esp-adf/components/audio_hal/driver/zl38063/api_lib /home/kapil/adf_kapil/esp-adf/components/audio_hal/driver/zl38063/example_apps /home/kapil/adf_kapil/esp-adf/components/audio_hal/driver/zl38063/firmware
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/audio_hal -laudio_hal -L/home/kapil/adf_kapil/esp-adf/components/audio_hal/driver/zl38063/firmware -lfirmware
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += audio_hal
component-audio_hal-build: 
