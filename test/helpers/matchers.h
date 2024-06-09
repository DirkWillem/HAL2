#pragma once

#include <gmock/gmock.h>

namespace matchers {

MATCHER_P(SpanOfSize, size, "") {
  return arg.size() == size;
}

}   // namespace matchers