#pragma once

namespace vrpc {

/**
 * Network topology of the vRPC network
 */
enum class NetworkTopology {
  /** Single client that communicates with a single server */
  SingleClientSingleServer,
  /** Single client that communicates with multiple servers */
  SingleClientMultipleServer,
};

struct VrpcNetworkConfig {
  NetworkTopology topology = NetworkTopology::SingleClientMultipleServer;
};

}   // namespace vrpc