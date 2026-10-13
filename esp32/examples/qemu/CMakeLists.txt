# NeuroClaw-ESP on Espressif QEMU, with a stub gate and a fake Telegram + LLM on
# the host (esp32/scripts/qemu_e2e.py). The real gate is NeuroEdge's ne_gate.
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(neuroclaw-qemu)
