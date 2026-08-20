#include <memory>

#include <random>

#include "../dht/dht_node.h"
#include "../handlers/tcp_listener.h"
#include "../handlers/udp_handler.h"
#include "../reactor/reactor.h"
#include "../utils/tcp_utils.h"


int main(int argc, char* argv[]) {


    Reactor reactor;

    bool is_client = (argc == 3 && std::string(argv[1]) == "--connect");

    int tcp_port = is_client ? 8081 : 8080;
    int udp_port = is_client ? 9091 : 9090;


    static uint64_t node_id = []() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        return gen();
    }();

    DHTNode dht(node_id, tcp_port, udp_port);



    auto listener = std::make_unique<TCPListener>(tcp_port, node_id, udp_port);

    auto udp_sock = std::make_unique<UDPHandler>(udp_port);

    listener->set_event_callback(
        [&reactor](std::unique_ptr<EventHandler> handler) {
            reactor.register_handler(std::move(handler));
        }
    );

    // reactor.register_handler(std::move(listener));
    reactor.register_handler(std::move(listener));
    reactor.register_handler(std::move(udp_sock));

    int dht_port = is_client ? 4223 : 4222;
    dht.start(dht_port);



    std::set<uint64_t> connected_peers;
    std::mutex peers_mutex;

    dht.discover([&reactor, &connected_peers, &peers_mutex, &dht, tcp_port, udp_port](const Peer& peer) {
        std::cout << "Discovered peer" << std::endl;
        std::cout << "node_id: " << peer.node_id << std::endl;
        std::cout << "tcp_port: " << peer.tcp_port << std::endl;
        std::cout << "udp_port: " << peer.udp_port << std::endl;
        std::cout << "ip: " << peer.ip << std::endl;

        std::lock_guard<std::mutex> lock(peers_mutex);
        if (peer.node_id == node_id) return;        // skip self by node_id
        if (peer.ip == dht.get_self_ip()) return;   // skip self by IP
        if (connected_peers.count(peer.node_id)) return;
        connected_peers.insert(peer.node_id);

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.ip, ip_str, sizeof(ip_str));

        connect_to_peer(reactor, ip_str, peer.tcp_port, node_id, tcp_port, udp_port);
    });

    dht.announce();


    for (;;) {
        reactor.handle_events();
        dht.try_set_public_ip();
    }

}
