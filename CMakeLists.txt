# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(adafruit_qt_py_rp2040-tsl2591)

target_sources(app PRIVATE src/main.c)
