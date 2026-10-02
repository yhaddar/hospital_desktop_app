cmake_minimum_required(VERSION 3.21)
project(HospitalSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

qt_add_executable(HospitalSim
    src/main.cpp
    src/mainwindow.h src/mainwindow.cpp
    src/database.h src/database.cpp
    src/simulationservice.h src/simulationservice.cpp
)
target_link_libraries(HospitalSim PRIVATE Qt6::Widgets Qt6::Sql)

