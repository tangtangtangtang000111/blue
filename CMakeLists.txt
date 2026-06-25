cmake_minimum_required(VERSION 3.16)
project(BluetoothCar VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Support both Qt5 and Qt6
find_package(Qt6 QUIET COMPONENTS Core Quick QuickControls2 Bluetooth)
if(Qt6_FOUND)
    message(STATUS "Using Qt6")
    set(QT_LIBS Qt6::Core Qt6::Quick Qt6::QuickControls2 Qt6::Bluetooth)
else()
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Quick QuickControls2 Bluetooth)
    message(STATUS "Using Qt5")
    set(QT_LIBS Qt5::Core Qt5::Quick Qt5::QuickControls2 Qt5::Bluetooth)
endif()

add_executable(BluetoothCar
    src/main.cpp
    src/bluetoothcontroller.cpp
    src/heatmapimageprovider.cpp
    qml.qrc
)

target_include_directories(BluetoothCar PRIVATE src)
target_link_libraries(BluetoothCar PRIVATE ${QT_LIBS})

# Android specific settings
if(ANDROID)
    set_target_properties(BluetoothCar PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_MIN_SDK_VERSION 26
        QT_ANDROID_TARGET_SDK_VERSION 33
    )
endif()
