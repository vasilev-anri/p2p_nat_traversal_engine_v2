cmake_minimum_required(VERSION 4.1)
project(p2p_nat_v2)

set(CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENDHT REQUIRED IMPORTED_TARGET opendht)

include_directories(src/protocol)

add_executable(p2p_nat_v2 src/app/main.cpp
        src/reactor/reactor.cpp
        src/reactor/reactor.h
        src/handlers/event_handler.h
        src/handlers/tcp_listener.cpp
        src/handlers/tcp_listener.h
        src/utils/unique_fd.h
        src/handlers/tcp_session.cpp
        src/handlers/tcp_session.h
        src/handlers/udp_handler.cpp
        src/handlers/udp_handler.h
        src/utils/error_utils.h
        src/utils/socket_utils.h
        src/utils/tcp_utils.h
        src/utils/socket_setup.h
        src/utils/io_events.h
        src/wire/msg_header.h
        src/wire/msg_types.h
        src/wire/msg.h
        src/wire/messages/hello.h
        src/wire/msg_codec.cpp
        src/wire/msg_codec.h
        src/wire/msg_parser.cpp
        src/wire/msg_parser.h
        src/peer/peer.h
        src/session/session_state.h
        src/session/session_role.h
        src/wire/messages/ping.h
        src/wire/messages/pong.h
        src/dht/dht_node.cpp
        src/dht/dht_node.h
        src/utils/codec_utils.h
        src/peer/peer_manager.h
        src/peer/peer_manager.cpp
        src/rendezvous/rendezvous_client.cpp
        src/rendezvous/vps_utils.h
)

target_link_libraries(p2p_nat_v2 PRIVATE PkgConfig::OPENDHT)
