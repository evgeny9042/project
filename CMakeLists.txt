cmake_minimum_required(VERSION 3.2)

project(pong  VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

set ( SOURCES
	main.cpp 	
	mainwindow.cpp 
	game.cpp 
	networkai.cpp 
	reinforceai.cpp 
	simpleai.cpp 
)

set ( HEADERS
	aiinterface.h 
	mainwindow.h 
	game.h 
	networkai.h 
	reinforceai.h 
	simpleai.h 
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(../dlib dlib_build)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable( pong ${SOURCES} ${HEADERS} )

set_target_properties(pong PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(pong Qt5::Widgets)
target_link_libraries(pong dlib::dlib)

install(TARGETS pong RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT egavrilin90@gmail.com)

include(CPack)