/*  Copyright 2009 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/common/asio_service.h"

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "boost/date_time/posix_time/posix_time.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

namespace bptime = boost::posix_time;

namespace maidsafe {

namespace test {

TEST_CASE("AsioService start and stop", "[AsioService][Unit]") {
  bool done(false);
  std::mutex mutex;
  std::condition_variable cond_var;

  auto task([&] {
    {
      std::lock_guard<std::mutex> lock(mutex);
      done = true;
    }
    cond_var.notify_one();
  });

  SECTION("Allocate no threads") {
    CHECK_THROWS_AS(AsioService(0), std::exception);
  }

  SECTION("Normal usage") {
    AsioService asio_service(2);
    asio_service.service().post(task);
    std::unique_lock<std::mutex> lock(mutex);
    CHECK(cond_var.wait_for(lock, std::chrono::milliseconds(100), [&] { return done; }));
    CHECK(done);

    SECTION("Explicit Stop before destruction") {
      CHECK_NOTHROW(asio_service.Stop());
      CHECK_NOTHROW(asio_service.Stop());
    }

    SECTION("Stop, then post more tasks") {
      done = false;
      asio_service.Stop();
      asio_service.service().post(task);
      CHECK_FALSE(cond_var.wait_for(lock, std::chrono::milliseconds(100), [&] { return done; }));
      CHECK_FALSE(done);
    }
  }
}

}  // namespace test

}  // namespace maidsafe
