#pragma once

#include <memory>
#include <queue>

namespace th {

class EventLoop;

class Event {
 public:
  virtual ~Event() = default;

  virtual void operator()(EventLoop&) = 0;
};

class EventLoop {
 public:
 private:
  std::queue<std::unique_ptr<Event>> events{};
};

}   // namespace th
