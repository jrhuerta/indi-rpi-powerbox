cmake_minimum_required(VERSION 3.13)
project(indi-rpi-pb CXX C)

include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/")

find_package(INDI REQUIRED)
find_package(Nova REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GSL REQUIRED)

set(CDRIVER_VERSION_MAJOR 1)
set(CDRIVER_VERSION_MINOR 2)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/indi_rpi_pb.xml.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/indi_rpi_pb.xml
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})
include_directories(${NOVA_INCLUDE_DIR})
include_directories(${EV_INCLUDE_DIR})

include(CMakeCommon)

# file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/rpi_powerbox.cpp)
set(GPIO_LIBRARIES "pigpiod_if2.so")

add_executable(indi_rpi_pb ${SOURCES})

# and link it to these libraries
target_link_libraries(
    indi_rpi_pb
    ${INDI_LIBRARIES}
    ${NOVA_LIBRARIES}
    ${GSL_LIBRARIES}
    ${GPIO_LIBRARIES}

)

# Install indi_rpi_pb
install(TARGETS indi_rpi_pb RUNTIME DESTINATION bin)
install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/indi_rpi_pb.xml
    DESTINATION ${INDI_DATA_DIR}
)
