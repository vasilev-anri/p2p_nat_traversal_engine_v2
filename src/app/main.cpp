#include <memory>

#include <random>

#include "../dht/dht_node.h"
#include "../handlers/tcp_listener.h"
#include "../handlers/udp_handler.h"
#include "../reactor/reactor.h"
#include "../rendezvous/rendezvous_client.h"
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
    auto udp_raw = udp_sock.get(); /* saving raw pointer before std::move() */

    // vps addr
    constexpr uint32_t VPS_IP = 0;
    constexpr uint16_t VPS_PORT = 9999;

    udp_sock->set_vps_address(inet_addr("62.238.114.216"), VPS_PORT);

    RendezvousClient rendezvous(*udp_raw, tcp_port, node_id, inet_addr("62.238.114.216"), VPS_PORT);

    udp_sock->set_rendezvous_callback([&rendezvous](const Notify& notify) {
        rendezvous.handle_notify(const_cast<Notify*>(&notify));
    });

    rendezvous.set_notify_callback([&](uint64_t target_node_id, Endpoint pub, Endpoint priv) {
        printf("Got peer endpoints - public: %s:%d\n", ip_to_str(pub.ip).c_str(), ntohs(pub.udp_port));
        printf("                     Private: %s:%d\n", ip_to_str(priv.ip).c_str(), ntohs(priv.udp_port));

        udp_raw->setup_punch(target_node_id, pub.ip, ntohs(pub.udp_port), priv.ip, ntohs(priv.udp_port));

        if (pub.ip != 0 && pub.tcp_port != 0)
            connect_to_peer(reactor, ip_to_str(pub.ip), ntohs(pub.tcp_port), node_id, tcp_port, udp_port);

        if (priv.ip != 0 && priv.tcp_port != 0)
            connect_to_peer(reactor, ip_to_str(priv.ip), ntohs(priv.tcp_port), node_id, tcp_port, udp_port);
    });

    udp_sock->set_punch_callback([](uint32_t ip, uint16_t port) {
        printf("Punch packet from %s:%d\n", ip_to_str(ip).c_str(), ntohs(port));
    });


    listener->set_event_callback(
        [&reactor](std::unique_ptr<EventHandler> handler) {
            reactor.register_handler(std::move(handler));
        }
    );

    // reactor.register_handler(std::move(listener));
    reactor.register_handler(std::move(listener));
    reactor.register_handler(std::move(udp_sock));

    rendezvous.send_register();

    int dht_port = is_client ? 4223 : 4222;
    dht.start(dht_port);



    std::set<uint64_t> connected_peers;
    std::mutex peers_mutex;

    dht.discover([&reactor, &connected_peers, &peers_mutex, &dht, tcp_port, udp_port, &rendezvous](const Peer& peer) {
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

        // ask rendezvous to coordinate punch
        rendezvous.send_request(peer.node_id);

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.ip, ip_str, sizeof(ip_str));

    });

    dht.announce();


    for (;;) {
        reactor.handle_events();
        dht.try_set_public_ip();
    }

}
