cmake_minimum_required(VERSION 3.5)

project(WeatherApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts Network Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts Network Test)


add_executable(WeatherApp
    main.cpp
)

target_link_libraries(WeatherApp PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::Network
)

add_executable(WeatherAppTests test_weather.cpp)
set_target_properties(WeatherAppTests PROPERTIES AUTOMOC ON)
target_link_libraries(WeatherAppTests PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Charts 
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Test
)

enable_testing()
add_test(NAME WeatherAppTests COMMAND $<TARGET_FILE:WeatherAppTests>)


if(APPLE)

    # set(MACOSX_BUNDLE_ICON_FILE myicon.icns)
    
    
    set_target_properties(WeatherApp PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.weatherapp"
        MACOSX_BUNDLE_BUNDLE_NAME "Weather App"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_LONG_VERSION_STRING "1.0.0"
    )
    
    
    set_target_properties(WeatherApp PROPERTIES
        MACOSX_BUNDLE_INFO_STRING "Weather Application"
        MACOSX_BUNDLE_COPYRIGHT "Copyright © 2023 Your Company"
    )
    
    
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(WeatherApp PRIVATE ${COCOA_LIBRARY})
endif()


if(APPLE AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET WeatherApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            $<TARGET_BUNDLE_DIR:WeatherApp>
            ${CMAKE_BINARY_DIR}/WeatherApp.app
    )
endif()
