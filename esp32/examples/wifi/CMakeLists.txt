# NeuroClaw-ESP on a Wi-Fi board (ESP32, -S3, -C3, -C6) with no devices of its
# own: the brain, its memory and schedule, the web, and the serial console. A
# NeuroEdge chip replaces the empty gate with ne_gate. CI builds it for each
# target (.github/workflows/esp32.yml).
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(neuroclaw-wifi)
