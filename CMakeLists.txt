project(audiodecoder.openmpt)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(Openmpt REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${OPENMPT_INCLUDE_DIRS})

set(OPENMPT_SOURCES src/OpenMptCodec.cpp)

set(DEPLIBS ${kodiplatform_LIBRARIES} ${OPENMPT_LIBRARIES})

build_addon(audiodecoder.openmpt OPENMPT DEPLIBS)

include(CPack)
