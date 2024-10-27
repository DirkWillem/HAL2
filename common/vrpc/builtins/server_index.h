#pragma once

#include <span>

#include <vrpc/generated/services/server_index.h>

#include <vrpc/proto_helpers.h>

namespace vrpc::server_index {

struct ServiceInfo {
  uint32_t         id;
  std::string_view identifier;
};

class ServerIndexImpl {
 public:
  explicit ServerIndexImpl()
      : service_ids{} {}

  constexpr void InitializeIds(std::span<const ServiceInfo> ids) noexcept {
    service_ids = ids;
  }

  constexpr void GetServiceCount([[maybe_unused]] const GetServiceCountRequest&,
                                 GetServiceCountResponse& response) noexcept {
    response.count = static_cast<uint32_t>(service_ids.size());
  }

  constexpr void GetServiceInfo(const GetServiceInfoRequest& request,
                                GetServiceInfoResponse& response) noexcept {
    if (request.index < service_ids.size()) {
      response.error = static_cast<vrpc_server_index_GetServiceInfoError>(
          GetServiceInfoError::None);
      response.service_id = service_ids[request.index].id;
      (void)WriteProtoString(service_ids[request.index].identifier,
                             response.service_identifier);
    } else {
      response.error = static_cast<vrpc_server_index_GetServiceInfoError>(
          GetServiceInfoError::IndexOutOfBounds);
    }
  }

 private:
  std::span<const ServiceInfo> service_ids;
};

static_assert(ServerIndex<ServerIndexImpl>);

}   // namespace vrpc::server_index