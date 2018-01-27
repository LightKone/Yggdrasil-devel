# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(Yggdrasil)

# get all *.c files recursively
file(GLOB_RECURSE SRC_FILES_PROTOS ${PROJECT_SOURCE_DIR}/protos/*.c)
#file(GLOB_RECURSE SRC_FILES_CORE_UTILS ${PROJECT_SOURCE_DIR}/utils/*.c)
file(GLOB_RECURSE SRC_FILES_CORE ${PROJECT_SOURCE_DIR}/core/*.c)

# include headers
include_directories(.)

include_directories(../Yggdrasil-LowLevelLib/)
include_directories(../Yggdrasil-LowLevelLib/src)

include_directories(/usr/include)
include_directories(/usr/include/uuid)
include_directories(/usr/include/libnl3)

#set(CMAKE_BUILD_TYPE RelWithDebInfo)
#set(CMAKE_BUILD_TYPE Debug)

#Set C flags (now compiling for with debug messages (-D DEBUG))
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D DEBUG -g -Wall -O0 -pthread -lnl-genl-3 -lnl-3 -luuid -lrt")
set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -lnl-genl-3 -lnl-3 -luuid -lrt")

#add libraries
add_library(ygg OBJECT ${SRC_FILES_PROTOS})
add_library(ygglllib OBJECT ${SRC_FILES_CORE})

add_dependencies(ygg ygglllib)

# add executables of demos
add_executable(MessageExchange $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> demos/MessageExchange.c)
add_executable(OneHopBcast $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> demos/OneHopBcast.c)
add_executable(OneHopBcastWithTimer $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> demos/OneHopBcastWithTimer.c)

# add executables of tests
add_executable(bloomfilterTest $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/bloomfilterTest.c)
add_executable(discoveryTest $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/discoveryTest.c)
add_executable(dispatcherTest $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/dispatcherTest.c)
add_executable(GLS_test $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/GLS_test.c)
add_executable(pushGossipDemo $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/pushGossipDemo.c)
add_executable(QueueTest $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/QueueTest.c)
add_executable(randomSelectTest $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/randomSelectTest.c)
add_executable(simpleAggTest $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/simpleAggTest.c)
add_executable(simpleAggWithBloomFilterTest $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/simpleAggWithBloomFilterTest.c)
add_executable(simpletcpclient_forAgg $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/simpletcpclient_forAgg.c)
add_executable(simpletcpclient_forAggWBF $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/simpletcpclient_forAggWBF.c)
add_executable(aggTest $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/aggregation_test.c)
add_executable(aggtcpclient $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/aggregation_tcpclient.c)
add_executable(ControlStructureTest $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tests/ControlStructureTest.c)

# add executables of tools (not available yet)
#add_executable(ControlProtocolClient $<TARGET_OBJECTS:ygg> $<TARGET_OBJECTS:ygglllib> $<TARGET_OBJECTS:ygglllib_obj> tools/control_protocol_client.c)
