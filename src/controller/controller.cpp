#include "controller.h"

#include "document.pb.h"

#include "../document/document.h"
#include "../utils.h"

#include <optional>
#include <tinyxml2/tinyxml2.h>

namespace {

void BuildSimpleResponse(std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         drogon::HttpStatusCode code = drogon::k400BadRequest) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    callback(resp);
}

std::optional<std::int64_t> GetTtlHeader(const std::string& value) try {

} catch (const std::exception& e) {
    LOG("Failed to get ttl header " << e.what(), ELogLevel::LL_WARN);
    return std::nullopt;
}

}
