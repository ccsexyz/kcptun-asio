cmake_minimum_required(VERSION 3.6)
project(kcptun)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

set(SOURCE_FILES
        ikcp.c
        ikcp.h
        local.cpp
        local.h
        main.cpp
        server.cpp
        server.h
        sess.cpp
        sess.h
        utils.h
        kcptun_server.h
        kcptun_server.cpp
        kcptun_client.h
        kcptun_client.cpp
        frame.h
        smux.h
        smux.cpp
        encrypt.h
        encrypt.cpp
        config.h
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
	reedsolomon.h)

add_executable(kcptun ${SOURCE_FILES})
target_link_libraries(kcptun pthread)
target_link_libraries(kcptun cryptopp)
target_link_libraries(kcptun snappy)
