
cmake_minimum_required(VERSION 3.0)

project(AgileOctopus C)

add_executable(agile.tariff agile.tariff.c)
add_executable(getTariffs getTariffs.c)

find_library(lib_curl curl)
find_library(lib_mariadb mariadb)
find_library(lib_json-c json-c)

list(APPEND EXTRA_INCLUDES "/usr/local/include/json")

include_directories("/usr/include/mariadb/mysql")
include_directories("/usr/include/mariadb")

target_link_libraries(getTariffs ${lib_curl} ${lib_mariadb} ${lib_json-c})
target_link_libraries(agile.tariff ${lib_mariadb})
