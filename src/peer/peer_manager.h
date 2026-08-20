#pragma once

#include <vector>

#include "peer.h"


std::vector<uint8_t> serialize(Peer& peer);
Peer deserialize(std::vector<uint8_t>& data);
