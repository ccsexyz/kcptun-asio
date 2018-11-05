cmake_minimum_required(VERSION 3.6)
project(kcptun)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DASIO_STANDALONE")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
include_directories("${CMAKE_SOURCE_DIR}/asio/asio/include")
include_directories("${CMAKE_SOURCE_DIR}/rapidjson/include/rapidjson")
include_directories("${CMAKE_SOURCE_DIR}/gflags/include")
include_directories("${CMAKE_SOURCE_DIR}/snappy")

add_subdirectory("glog")
add_subdirectory("gflags")
add_subdirectory("snappy")

set(SOURCE_FILES
        ikcp.c
        ikcp.h
        sess.cpp
        sess.h
        utils.h
        utils.cpp
        frame.h
        smux.h
        smux.cpp
        encrypt.h
        encrypt.cpp
        config.h
        config.cpp
        snappy_stream.cpp
        snappy_stream.h
        fec.cpp
	fec.h
	galois.cpp
	galois.h
	galois_noasm.cpp
	galois_noasm.h
	galois_table.c
	inversion_tree.cpp
	inversion_tree.h
	matrix.cpp
	matrix.h
	reedsolomon.cpp
	reedsolomon.h
        async_fec.cpp
        async_fec.h)

set(KCPTUN_CLIENT_SOURCE_FILES ${SOURCE_FILES} 
        kcptun_client_main.cpp
        kcptun_client.h
        kcptun_client.cpp
        local.cpp
        local.h)

set(KCPTUN_SERVER_SOURCE_FILES ${SOURCE_FILES}
        kcptun_server_main.cpp
        kcptun_server.h
        kcptun_server.cpp
        server.cpp
        server.h)

add_executable(kcptun_client ${KCPTUN_CLIENT_SOURCE_FILES})
target_link_libraries(kcptun_client glog)
target_link_libraries(kcptun_client gflags)
target_link_libraries(kcptun_client snappy)
if(UNIX)
        target_link_libraries(kcptun_client "${CMAKE_SOURCE_DIR}/cryptopp/libcryptopp.a")
        target_link_libraries(kcptun_client -static-libstdc++)
        target_link_libraries(kcptun_client pthread)
else()
        target_link_libraries(kcptun_client "${CMAKE_SOURCE_DIR}/cryptopp/cryptlib.lib")
endif()

add_executable(kcptun_server ${KCPTUN_SERVER_SOURCE_FILES})
target_link_libraries(kcptun_server glog)
target_link_libraries(kcptun_server gflags)
target_link_libraries(kcptun_server snappy)
if(UNIX)
        target_link_libraries(kcptun_server "${CMAKE_SOURCE_DIR}/cryptopp/libcryptopp.a")
        target_link_libraries(kcptun_server -static-libstdc++)
        target_link_libraries(kcptun_server pthread)
else()
        target_link_libraries(kcptun_server "${CMAKE_SOURCE_DIR}/cryptopp/cryptlib.lib")
endif()
