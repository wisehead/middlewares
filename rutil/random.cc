//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//

#include "rutil/random.h"

#include <stdint.h>
#include <string.h>
#include <thread>
#include <utility>

#include "rport/likely.h"
#include "rocksutil/thread_local.h"

#if ROCKSUTIL_SUPPORT_THREAD_LOCAL
#define STORAGE_DECL static __thread
#else
#define STORAGE_DECL static
#endif

namespace rocksutil {

Random* Random::GetTLSInstance() {
  STORAGE_DECL Random* tls_instance;
  STORAGE_DECL std::aligned_storage<sizeof(Random)>::type tls_instance_bytes;

  auto rv = tls_instance;
  if (UNLIKELY(rv == nullptr)) {
    size_t seed = std::hash<std::thread::id>()(std::this_thread::get_id());
    rv = new (&tls_instance_bytes) Random((uint32_t)seed);
    tls_instance = rv;
  }
  return rv;
}

}  // namespace rocksutil
