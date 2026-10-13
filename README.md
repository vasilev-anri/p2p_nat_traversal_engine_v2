# P2P NAT Traversal Engine

A peer-to-peer Network Address Translation (NAT) traversal and connection-establishment engine



Devices behind home Wi-Fi or mobile networks often cannot connect directly to each other. This project helps two peers find and establish a direct connection, working around the obstacles that normally block them.



NAT traversal is a critically important issue in modern real-time internet communication, not a solved or trivial one. This project addresses it with Distributed Hash Table (DHT)-based peer discovery (no fixed peer list, no manual IP exchange), an authenticated rendezvous protocol that coordinates the connection using HMAC, and UDP hole punching combined with TCP connect. It has been tested on real hardware - Between a local machine and a VPS, and between two machines on separate mobile networks in different countries - not just in theory. This is the connection-establishment layer: infrastructure for getting peers talking, not a full chat or file-sharing application.



## Features

- Event Poll-based single-threaded reactor (Edge-Triggered Notification mode)
- DHT-based peer discovery (Kademlia OpenDHT)
- Authenticated rendezvous protocol (HMAC-SHA256 with replay protection)
- UDP hole punching
- Outbound TCP connection establishment
- NAT keep-alive (periodic refresh so mappings don't expire)


NAT is used to modify source and/or destination IP addresses and/or ports. It enables private networks to use the Internet or the cloud. In other words, it translates private IP addresses in a private network to a public network address before packets are sent to an external network. It can also translate port numbers, allowing multiple devices on a private network to share the same public IP address.


The NAT device creates a state/translation entry: private endpoint <-> public endpoint.
If NAT finds a public address-to-private address match in the entry, it translates the destination and forwards the packet.

As for unsolicited incoming packets, if NAT is unable to find a translation entry telling which private host should receive a packet, the NAT device cannot perform the required translation; consequently, the packet is dropped

![NAPT traversal process - success](docs/images/nat-success-diagram.png)
![NAPT traversal process - failure](docs/images/nat-failure-drop-diagram.png)

Even when peers behind NATs know each other's public endpoints, they cannot necessarily connect directly because each NAT may reject incoming traffic from the other peer until an appropriate NAT mapping has been established. 

If a peer's NAT encounters a packet before it has sent any traffic through it to that specific peer, then that packet may be treated as unsolicited incoming traffic and will be dropped.

UDP hole punching exploits outbound traffic to establish the necessary NAT mappings and filtering state, allowing subsequent packets from the other peer to pass through.


![UDP hole punching process, adapted from Ford, Srisuresh & Kegel (2005)](docs/images/hole-punching-diagram.png)

* Adapted from Figure 5 in Ford, Srisuresh & Kegel (2005)
* The red path shows one possible dropped first packet, if it arrives before that peer's own outbound packet has opened its NAT's hole. Since both peers keep sending, each side's hole still opens shortly after, and once both are open, communication proceeds normally in both directions.

## Establishing a connection between peers A and B

### UDP session & Punch

Both peers must have an active UDP *session* with the rendezvous server S. To accomplish that, both clients must register themselves with the server by sending a REGISTER message and letting the server know about their private endpoint (IP and port), so that the server will have both the private endpoint and the public endpoint; the latter is acquired from the translated source endpoint of the packet as observed by the server (usually, the public IP and UDP port that the NAPT assigned to the flow/mapping). If a peer is not behind a NAT/NAPT, its private and public endpoints should be identical [1].

Here, session refers to a software-managed "session built on top of connectionless UDP datagram sockets. The client periodically sends `KEEPALIVE` messages(heartbeat packets) to the server to keep NAT mappings open, thus letting it know that the peer is still alive, and keep NAT/NAPT port mappings alive [1], even though UDP itself establishes no persistent connection or session.

Once an engine knows its public IP, it announces itself on the DHT under the room key and listens for other peers announcing under the same key (see the DHT section below). When an engine discovers a peer, it learns that peer's node ID and sends a `REQUEST` to the rendezvous server S, containing its own private endpoint and the target's node ID. Both peers do this independently and at about the same time, so there is no single initiator and launch order doesn't matter. S already holds both peers' endpoints from registration, and replies with a `NOTIFY` to each peer carrying the other's public and private endpoints.

On `NOTIFY`, each engine starts sending UDP packets to both of the other peer's endpoints (the punch) and, at the same moment, attempts a plain outbound TCP connection to the peer's public endpoint.

### TCP Connection

On the same `NOTIFY`, the engine also opens a plain outbound TCP connection to the peer. Both peers dial at the same time, so a session can form in whichever direction the NATs allow. This is not TCP hole punching as described in [1], Section 4, which needs both sides to connect from the same local port so the SYN packets cross.

In practice, a NATed peer connecting to a peer with a public IP works, because its NAT treats the connection as outbound. This is the laptop-to-VPS case that was tested. Between two peers behind separate NATs, no session formed in any test run (see Test results).

Once connected, the two sides exchange a short `HELLO` handshake and then send `PING` and `PONG` messages to confirm that the channel works. This protocol is only a proof of connectivity and not an application protocol.



## Peer discovery with a DHT

The DHT is used solely for peer discovery and has nothing to do with establishing the connection itself. It only lets peers find out that other peers exist, and it provides no mechanism for connectivity, even though it uses UDP under the hood. In this section, "peer" and "node" mean the same thing: a participant in the network.

The engine builds on Kademlia [2], a proven design: BitTorrent's peer discovery is based on it, as are several other large peer-to-peer networks (eMule's Kad network, and libp2p, the networking layer behind IPFS). It uses OpenDHT, a C++ implementation of the Kademlia DHT [3].

The peer joins the network through a public bootstrap node, `bootstrap.jami.net:4222`, the same one used in OpenDHT's own examples [3]. A node can join through any node that is already connected [3], and a Kademlia node needs one such contact to get in [2], so there is no fixed list of peers, but there is one fixed entry point.

The room key `P2P_ROOM_KEY` is hashed into the 160-bit DHT key under which records are stored [3]. It works like a channel name: peers with the same key find each other, and peers with a different key never see each other's records. It is not a secret, and the records are neither signed nor encrypted.

Once the node knows its public IP, it announces itself by publishing a record containing its node ID, public IP, and local TCP & UDP ports, and it listens on the same key. Records expire after approximately 10 minutes unless refreshed [3]; The engine currently announces once. When another peer's record appears, the engine ignores its own record and any record without an IP, and treats the rest as a discovered peer. The record is not enough to connect: the ports in it are local, and the public port is known only to the rendezvous server, so the next step is a `REQUEST` message to that server. OpenDHT runs on its own thread and calls the discovery callback from it [3].



## Architecture

The architecture employs a non-blocking, event-driven I/O model leveraging Linux epoll [4]: a single thread waits for I/O events on many sockets and dispatches each event to the handler that owns the socket, an arrangement known as the reactor pattern [5]. The pattern keeps event detection separate from the code that reacts to events. Each handler (a TCP listener, a UDP handler for rendezvous and hole-punching traffic, and one session handler per TCP connection) has a single responsibility, and a new kind of handler can be added without changing the loop. It also avoids multithreading in the engine's own network code, at the price that a handler must never block or hog the loop [5]. Sockets are non-blocking and registered in edge-triggered mode, so a handler must drain its socket until `EAGAIN` or it misses data [4]. Writes go through a per-session queue, and `EPOLLOUT` is requested only while the queue is non-empty, so a slow peer cannot block the loop. Timers (keep-alives, pings, punch retries) are polled once per loop iteration. Concurrent TCP sessions are capped at 256 and punch targets at 128. OpenDHT runs on its own thread and reports discovered peers through a callback, whose set of already-requested peers is guarded by a mutex.

![Architecture. Main thread: reactor & handlers; DHT Thread](docs/images/architecture-diagram.png)

Engine components and the traffic flow between them. The reactor and `RendezvousClient` run on the main thread. OpenDHT runs on its own thread and invokes the discovery callback, which calls `RendezvousClient::send_request()` to send the request message.

## Test results

| Setup      | UDP Path | TCP Session     |
| :---:        |    :----:   |          :---: |
| Computer behind a NAT & a VPS with a public IP      | established       | established   |
| Two computers on separate mobile networks (4 runs)   | established in 4 out of 4        | not established in 4 out of 4      |

Logs from one side of the last mobile-to-mobile run, with addresses replaced with placeholders.

```
[info] node_id: <node A>
[info] tcp: 8080 udp: 9090
[info] rendezvous server: <server ip>:9999
[info] public ip: <A public ip>
[dht] discovered peer - node_id: <node B> ip: <B public ip>
[rendezvous] NOTIFY received from node: <node B>
[rendezvous] peer endpoints - public: <B public ip>:9090 private: <B private ip>:9090
[tcp] dialing <B public ip>:8080
[punch] hole opened for node <node B> <-- <B public ip>:9090
[tcp] outbound connect failed: Connection refused
[rendezvous] sending keepalive - node_id: <node A>
```

UDP Path is established when each peer receives a punch packet from the other. The TCP Session is established when the `HELLO` handshake is completed.

The UDP punch worked in all four runs. The TCP connection did not form in any of them, which is expected: the engine makes only a plain outbound connect, so each peer's connection attempt arrives at the other's NAT with no matching outgoing session, and a traditional NAT treats such traffic as unsolicited [1]. The logs are consistent with this but do not prove it. Getting a session between two NATed peers would need TCP hole punching or a relay (see Roadmap).

## Limitations

**Limited testing.** Tested between NATed computers and a public VPS, and between two machines on separate mobile networks. Other networks have not been tested; the NAT types are not detected, and no automated tests.

**No session across two NATs.** The UDP punch succeeds, but the TCP connection does not get established, and nothing uses the punched UDP path. There is no relay fallback.

**Authentication.** The shared-secret HMAC covers only rendezvous messages, and it gives no per-peer identity or revocation. Punch packets (any datagram from a known peer endpoint counts as success), the `HELLO` node ID and the DHT records are unauthenticated, and the records are unencrypted. The room key is not a secret.

**Clock dependency.** Rendezvous messages carry a timestamp, and the receiver rejects any message more than 30 seconds away from its own clock, on both the server and the engine. The log says only "dropped unauthenticated packet", not that the clock was the reason.

**No peer selection API.** Currently, there is no way for the application to choose which peer to connect to.



## Roadmap

**Relay fallback.** Forward traffic through the server for peers that are unable to connect directly.

**Per-node authentication.** Replacing the shared-secret HMAC with per-node Ed25519 identities.

**Peer selection and a library API.** Letting the application choose which peer to connect to, and exposing the engine as a library with a clear interface.

**Wider evaluation.** Testing across various NAT types.


## References

[1] B. Ford, P. Srisuresh, and D. Kegel, "Peer-to-peer communication across network address translators," in Proc. USENIX Annual Technical Conference, 2005, pp. 179–192.  
[2] P. Maymounkov and D. Mazières, "Kademlia: A peer-to-peer information system based on the XOR metric," in Peer-to-Peer Systems: First International Workshop, IPTPS 2002, P. Druschel, F. Kaashoek, and A. Rowstron, Eds., ser. Lecture Notes in Computer Science, vol. 2429. Springer, 2002, pp. 53–65.  
[3] Savoir-faire Linux, "OpenDHT: A lightweight C++17 Distributed Hash Table implementation", GitHub repository. [Online]. Available: https://github.com/savoirfairelinux/opendht. [Accessed: Oct. 11, 2026].    
[4] M. Kerrisk, "THE LINUX PROGRAMMING INTERFACE, A Linux and UNIX® System Programming Handbook", San Francisco, CA, No Starch Press, Inc. 2010, ch. 63, pp. 1355-1367.   
[5] A. Petersen, "Patterns in C – Part 5: REACTOR," CVu, vol. 17, no. 5, Oct. 2005. [Online]. Available: https://www.adamtornhill.com/Patterns%20in%20C%205,%20REACTOR.pdf

## Acknowledgements

- My introduction to socket programming was *Beej's Guide to Network Programming Using Internet Sockets* by Brian "Beej" Hall (Revision 2.3.1, 2001), and I'm grateful to the author for such a clear, freely available resource.

- [RFC 3022](https://www.rfc-editor.org/info/rfc3022/): P. Srisuresh and K. Egevang, "Traditional IP Network Address Translator (Traditional NAT)," January 2001.
- [RFC 5128](https://www.rfc-editor.org/info/rfc5128/#section-3.3.1): P. Srisuresh, B. Ford, and D. Kegel, "State of Peer-to-Peer (P2P) Communication across Network Address Translators (NATs)," March 2008.
- [RFC 9293](https://datatracker.ietf.org/doc/html/rfc9293): W. Eddy, Ed., "Transmission Control Protocol (TCP)," August 2022.
- [epoll(7) — Linux manual page](https://man7.org/linux/man-pages/man7/epoll.7.html): Linux man-pages project, "epoll - I/O event notification facility," man-pages 6.19
