cmake_minimum_required(VERSION 3.10)
project(DecodificadorPRT7)

set(CMAKE_CXX_STANDARD 11)

add_executable(prtdcd main.cpp)

if(UNIX)
    # Librerías necesarias para serial en Linux
endif()
