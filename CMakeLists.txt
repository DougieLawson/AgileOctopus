
cmake_minimum_required(VERSION 3.0)

project(AgileOctopus C CXX)

add_executable(agile.tariff agile.tariff.c)
add_executable(getTariffs getTariffs.c)

add_executable(getConsumption getConsumption.c)
add_executable(getUP getUP.c)
add_executable(price price.c)
add_executable(tariff-bands tariff-bands.c)
add_executable(tariff-json tariff-json.c)
add_executable(tariff-json-S3 tariff-json.sqlite3.c)
add_executable(tariff-minrate tariff-minrate.c)
add_executable(tariff-now tariff-now.c)
add_executable(usage_price usage_price.c)
add_executable(usagetariff usagetariff.c)
add_executable(minRateWait mqttMessage.cpp minRateWait.cpp)

find_library(lib_curl curl)
find_library(lib_mariadb mariadb)
find_library(lib_json-c json-c)
find_library(lib_mosquitto mosquitto)
find_library(lib_mosquittopp mosquittopp)
find_library(lib_config config)
find_library(lib_sqlite sqlite3)
find_library(lib_pcre pcre)

list(APPEND EXTRA_INCLUDES "/usr/local/include/json")

include_directories("/usr/include/mariadb/mysql")
include_directories("/usr/include/mariadb")

target_link_libraries(getTariffs ${lib_curl} ${lib_mariadb} ${lib_json-c})
target_link_libraries(agile.tariff ${lib_mariadb})
target_link_libraries(minRateWait ${lib_mariadb} ${lib_mosquitto} ${lib_mosquittopp})
target_link_libraries(tariff-now ${lib_mosquitto} ${lib_mariadb} ${lib_config})
target_link_libraries(tariff-minrate ${lib_mariadb} ${lib_config})
target_link_libraries(tariff-json ${lib_mariadb} ${lib_config})
target_link_libraries(usage_price ${lib_mariadb} ${lib_config})
target_link_libraries(getTariffs ${lib_mariadb} ${lib_config})
target_link_libraries(usagetariff ${lib_mariadb} ${lib_config})
target_link_libraries(tariff-json-S3 ${lib_sqlite})
target_link_libraries(getUP ${lib_curl} ${lib_config} ${lib_pcre})
target_link_libraries(getConsumption ${lib_config} ${lib_json-c} ${lib_pcre} ${lib_mariadb} ${lib_curl})
target_link_libraries(price ${lib_config} ${lib_mariadb} ${lib_curl})
target_link_libraries(tariff-bands ${lib_config} ${lib_mariadb})
