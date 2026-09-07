#include <memory>

#include <random>

#include "../dht/dht_node.h"
#include "../handlers/tcp_listener.h"
#include "../handlers/udp_handler.h"
#include "../reactor/reactor.h"
#include "../rendezvous/rendezvous_client.h"
#include "../utils/tcp_utils.h"



void print_startup_logs(uint64_t node_id, uint16_t tcp_port, uint16_t udp_port, const char* vps_ip, uint16_t vps_port) {
    printf("[info] starting P2P NAT traversal engine\n");
    printf("[info] node_id: %lu\n", node_id);
    printf("[info] tcp: %d udp: %d\n", tcp_port, udp_port);
    printf("[info] rendezvous server: %s:%d\n", vps_ip, vps_port);
}


int main(int argc, char* argv[]) {

    Reactor reactor;

    int tcp_port = 8080;
    int udp_port = 9090;

    // vps addr
    const char* vps_ip = "62.238.114.216";
    constexpr uint16_t VPS_PORT = 9999;


    static uint64_t node_id = []() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        return gen();
    }();

    DHTNode dht(node_id, tcp_port, udp_port);


    auto listener = std::make_unique<TCPListener>(tcp_port, node_id, udp_port);

    auto udp_sock = std::make_unique<UDPHandler>(udp_port);
    auto udp_raw = udp_sock.get(); /* saving raw pointer before std::move() */




    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--vps" && i + 1 < argc) vps_ip = argv[i + 1];
    }

    print_startup_logs(node_id, tcp_port, udp_port, vps_ip, VPS_PORT);

    udp_sock->set_vps_address(inet_addr(vps_ip), VPS_PORT);

    RendezvousClient rendezvous(*udp_raw, tcp_port, node_id, inet_addr(vps_ip), VPS_PORT);

    udp_sock->set_rendezvous_callback([&rendezvous](const Notify& notify) {
        rendezvous.handle_notify(const_cast<Notify*>(&notify));
    });

    std::set<uint64_t> connected_peers;
    std::set<uint64_t> notified_peers;
    std::mutex peers_mutex;

    rendezvous.set_notify_callback([&](uint64_t target_node_id, Endpoint pub, Endpoint priv) {

        {
            std::lock_guard<std::mutex> lock(peers_mutex);
            if (notified_peers.contains(target_node_id)) return;
            notified_peers.insert(target_node_id);
        }



        udp_raw->setup_punch(target_node_id, pub.ip, ntohs(pub.udp_port), priv.ip, ntohs(priv.udp_port));

        if (pub.ip != 0 && pub.tcp_port != 0)
            connect_to_peer(reactor, ip_to_str(pub.ip), ntohs(pub.tcp_port), node_id, tcp_port, udp_port);

        // if (priv.ip != 0 && priv.tcp_port != 0)
        //     connect_to_peer(reactor, ip_to_str(priv.ip), ntohs(priv.tcp_port), node_id, tcp_port, udp_port);

        printf("[rendezvous] peer endpoints - public: %s:%d private: %s:%d\n",
            ip_to_str(pub.ip).c_str(), ntohs(pub.udp_port),
            ip_to_str(priv.ip).c_str(), ntohs(priv.udp_port));

    });

    udp_sock->set_punch_callback([udp_raw](uint32_t ip, uint16_t port) {
        udp_raw->mark_punch_success(ip, port);
    });


    listener->set_event_callback(
        [&reactor](std::unique_ptr<EventHandler> handler) {
            reactor.register_handler(std::move(handler));
        }
    );

    reactor.register_handler(std::move(listener));
    reactor.register_handler(std::move(udp_sock));

    rendezvous.send_register();

    int dht_port = 4222;
    dht.start(dht_port);





    dht.discover([&reactor, &connected_peers, &peers_mutex, &dht, tcp_port, udp_port, &rendezvous](const Peer& peer) {
        printf("[dht] discovered peer - node_id: %lu ip: %s\n", peer.node_id, ip_to_str(peer.ip).c_str());
        std::lock_guard<std::mutex> lock(peers_mutex);
        if (peer.node_id == node_id) return;        // skip self by node_id
        if (peer.ip == dht.get_self_ip()) return;   // skip self by IP
        if (connected_peers.count(peer.node_id)) return;
        connected_peers.insert(peer.node_id);

        // ask rendezvous to coordinate punch
        rendezvous.send_request(peer.node_id);
    });

    dht.announce();


    for (;;) {
        reactor.handle_events();
        dht.try_set_public_ip();
    }

}
