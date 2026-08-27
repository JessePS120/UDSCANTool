cmake_minimum_required(VERSION 3.16)
project(uds_can_device_tests C)

# Off-target build (NFR-4): links only App/ sources, never Core/ or
# Drivers/, so ISO-TP/UDS logic is exercised without any STM32 HAL.
set(APP_INC ${CMAKE_CURRENT_SOURCE_DIR}/../App/Inc)
set(APP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../App/Src)

add_executable(test_isotp
    isotp/test_isotp.c
    ${APP_SRC}/isotp.c
)
target_include_directories(test_isotp PRIVATE ${APP_INC})

add_executable(test_uds_client
    uds_client/test_uds_client.c
    ${APP_SRC}/uds_client.c
)
target_include_directories(test_uds_client PRIVATE ${APP_INC})

enable_testing()
add_test(NAME isotp COMMAND test_isotp)
add_test(NAME uds_client COMMAND test_uds_client)
