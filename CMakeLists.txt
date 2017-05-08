cmake_minimum_required(VERSION 3.6)
project(kcptun)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

set(SOURCE_FILES
        ikcp.c
        ikcp.h
        sess.cpp
        sess.h
        utils.h
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
        async_fec.h
        logger.h
        logger.cpp)

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
target_link_libraries(kcptun_client pthread)
target_link_libraries(kcptun_client cryptopp)
target_link_libraries(kcptun_client snappy)

add_executable(kcptun_server ${KCPTUN_SERVER_SOURCE_FILES})
target_link_libraries(kcptun_server pthread)
target_link_libraries(kcptun_server cryptopp)
target_link_libraries(kcptun_server snappy)
