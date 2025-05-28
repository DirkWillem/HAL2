module;

#include <cstdint>
#include <span>
#include <string_view>

export module vrpc.server.builtins:server_index;

import vrpc.protocol.server_index.server;
import vrpc.common;

namespace vrpc::builtins::server_index {

export struct ServiceInfo {
  std::uint32_t    id;
  std::string_view identifier;
};

export class ServerIndexImpl {
 public:
  explicit ServerIndexImpl() noexcept
      : service_ids{} {}

  constexpr void InitializeIds(std::span<const ServiceInfo> ids) noexcept {
    service_ids = ids;
  }

  constexpr void GetServiceCount(
      [[maybe_unused]] const ::vrpc::server_index::GetServiceCountRequest&,
      ::vrpc::server_index::GetServiceCountResponse& response) noexcept {
    response.count = static_cast<std::uint32_t>(service_ids.size());
  }

  constexpr void GetServiceInfo(
      const ::vrpc::server_index::GetServiceInfoRequest& request,
      ::vrpc::server_index::GetServiceInfoResponse&      response) noexcept {
    if (request.index < service_ids.size()) {
      // response.error = static_cast<vrpc_server_index_GetServiceInfoError>(
      //     GetServiceInfoError::None);
      MapEnumInto(::vrpc::server_index::GetServiceInfoError::None,
                  response.error);
      response.service_id = service_ids[request.index].id;
      (void)WriteProtoString(service_ids[request.index].identifier,
                             response.service_identifier);
    } else {
      MapEnumInto(::vrpc::server_index::GetServiceInfoError::IndexOutOfBounds,
                  response.error);
    }
  }

 private:
  std::span<const ServiceInfo> service_ids;
};

static_assert(::vrpc::server_index::ServerIndex<ServerIndexImpl>);

}   // namespace vrpc::builins::server_index