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

#exclude files
list(REMOVE_ITEM SRC_FILES_PROTOS ${PROJECT_SOURCE_DIR}/protos/aggregation/push_sum.c)

#add libraries
add_library(ygg OBJECT ${SRC_FILES_PROTOS})
add_library(ygglllib OBJECT ${SRC_FILES_CORE})

add_dependencies(ygg ygglllib)

# add executables of demos
add_executable(MessageExchange $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> demos/MessageExchange.c)
add_executable(OneHopBcast $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> demos/OneHopBcast.c)
add_executable(OneHopBcastWithTimer $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> demos/OneHopBcastWithTimer.c)

# add executables of tests
#add_executable(bloomfilterTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/bloomfilterTest.c)
#add_executable(discoveryTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/discoveryTest.c)
#add_executable(dispatcherTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/dispatcherTest.c)
#add_executable(GLS_test $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/GLS_test.c)
#add_executable(pushGossipDemo $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/pushGossipDemo.c)
#add_executable(QueueTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/QueueTest.c)
#add_executable(randomSelectTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/randomSelectTest.c)
#add_executable(simpleAggTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/simpleAggTest.c)
#add_executable(simpleAggWithBloomFilterTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/simpleAggWithBloomFilterTest.c)
#add_executable(simpletcpclient_forAgg $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/simpletcpclient_forAgg.c)
#add_executable(simpletcpclient_forAggWBF $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/simpletcpclient_forAggWBF.c)
#add_executable(aggTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/aggregation_test.c)
add_executable(YggdrasilControlProcess $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/ControlStructureTest.c)
#add_executable(reliableTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/reliableTest.c)
#add_executable(pushsumTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/push_sum_test.c)
add_executable(aggFlowTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/flow_updating_test.c)
#add_executable(drgTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/drg_test.c)
add_executable(aggGapTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/gap_test.c)
add_executable(aggGapBcastTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/gap_bcast_test.c)
add_executable(aggLimoTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/limosense_test.c)
add_executable(aggMultiTest $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/multi_root_test.c)


#add executables of utils
#add_executable(aggtcpclient $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/aggregation_tcpclient.c)
add_executable(tcpclient $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tests/tcpclient.c)

# add executables of tools
add_executable(ControlProtocolClient $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/control_protocol_client.c)
add_executable(cmdbuildtree $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/buildtree.c)
add_executable(cmdchecktree $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/checktree.c)
add_executable(cmdgetneighbors $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/getneighbors.c)
add_executable(cmdexecuteexperience $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/beginexperience.c)
add_executable(cmdterminateexperience $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/terminateexperience.c)
add_executable(cmdchangevalue $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/changevalue.c)
add_executable(cmdchangelink $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/changelink.c)
add_executable(cmdreboot $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/reboot.c)
add_executable(cmdshutdown $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/shutdown.c)
add_executable(cmdenablediscovery $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/enableannounces.c)
add_executable(cmddisablediscovery $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/disableannounces.c)
add_executable(cmddebug $<TARGET_OBJECTS:lkprotos> $<TARGET_OBJECTS:lkcore> $<TARGET_OBJECTS:lkbaseobj> tools/debugneighbors.c)
