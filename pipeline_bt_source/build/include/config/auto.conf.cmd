deps_config := \
	/home/kapil/idf_kapil/esp-idf/components/app_trace/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/aws_iot/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/bt/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/driver/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/esp32/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/esp_adc_cal/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/esp_event/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/esp_http_client/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/esp_http_server/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/ethernet/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/fatfs/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/freemodbus/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/freertos/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/heap/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/libsodium/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/log/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/lwip/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/mbedtls/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/mdns/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/mqtt/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/nvs_flash/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/openssl/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/pthread/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/spi_flash/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/spiffs/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/tcpip_adapter/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/vfs/Kconfig \
	/home/kapil/idf_kapil/esp-idf/components/wear_levelling/Kconfig \
	/home/kapil/adf_kapil/esp-adf/components/audio_board/Kconfig.projbuild \
	/home/kapil/idf_kapil/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/kapil/adf_kapil/esp-adf/components/esp-adf-libs/Kconfig.projbuild \
	/home/kapil/idf_kapil/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/kapil/adf_kapil/pipeline_bt_source/main/Kconfig.projbuild \
	/home/kapil/idf_kapil/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/kapil/idf_kapil/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
