// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "base/pmr/memory_resource.h"
#include "util/connection.h"
#include "util/fibers/synchronization.h"

namespace util {

class ListenerInterface;
class ProactorPool;

class AcceptServer {
  AcceptServer(const AcceptServer&) = delete;
  void operator=(const AcceptServer&) = delete;

 public:
  explicit AcceptServer(ProactorPool* pool, bool break_on_int = true)
      : AcceptServer(pool, nullptr, break_on_int) {
  }

  AcceptServer(ProactorPool* pool, PMR_NS::memory_resource* mr, bool break_on_int);

  ~AcceptServer();

  void Run();

  // If wait is false - does not wait for the server to stop.
  // Then you need to run Wait() to wait for proper shutdown.
  void Stop(bool wait = false);

  void Wait();

  // Returns the port number to which the listener was bound.
  // Takes ownership of the listener.
  // Check-fails in case of an error.
  uint16_t AddListener(uint16_t port, ListenerInterface* listener);

  // Advanced version that allows to specify bind address.
  // bind_addr can be null, in that case the behavior is to bind on all interfaces.
  // Takes ownership of the listener.
  // Does not check-fail - it's responsibility of the caller to check the error code.
  std::error_code AddListener(const char* bind_addr, uint16_t port, ListenerInterface* listener);

  // Adds a listener on unix domain sockets.
  // Takes ownership of the listener.
  std::error_code AddUDSListener(const char* path, mode_t permissions, ListenerInterface* listener);

  void TriggerOnBreakSignal(std::function<void()> f) {
    on_break_hook_ = std::move(f);
  }

  void set_back_log(uint16_t backlog) {
    backlog_ = backlog;
  }

 private:
  void BreakListeners();

  ProactorPool* pool_;
  PMR_NS::memory_resource* mr_;

  // Called if a termination signal has been caught (SIGTERM/SIGINT).
  std::function<void()> on_break_hook_;

  std::vector<std::unique_ptr<ListenerInterface>> list_interface_;
  fb2::BlockingCounter ref_bc_;  // to synchronize listener threads during the shutdown.

  bool was_run_ = false;
  bool break_on_int_;

  uint16_t backlog_ = 128;
};

}  // namespace util
