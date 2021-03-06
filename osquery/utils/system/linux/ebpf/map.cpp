/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under both the Apache 2.0 license (found in the
 *  LICENSE file in the root directory of this source tree) and the GPLv2 (found
 *  in the COPYING file in the root directory of this source tree).
 *  You may select, at your option, one of the above-listed licenses.
 */

#include <osquery/utils/system/linux/ebpf/map.h>
#include <osquery/utils/system/linux/ebpf/ebpf.h>

#include <boost/io/detail/quoted_manip.hpp>

#include <cerrno>
#include <cstring>

namespace osquery {
namespace ebpf {

namespace impl {

Expected<int, MapError> mapCreate(enum bpf_map_type map_type,
                                  std::size_t key_size,
                                  std::size_t value_size,
                                  std::size_t max_entries) {
  union bpf_attr attr;
  memset(&attr, 0, sizeof(union bpf_attr));
  attr.map_type = map_type;
  attr.key_size = static_cast<std::uint32_t>(key_size);
  attr.value_size = static_cast<std::uint32_t>(value_size);
  attr.max_entries = static_cast<std::uint32_t>(max_entries);
  auto exp_bpf = syscall(BPF_MAP_CREATE, &attr);
  if (exp_bpf.isError()) {
    return createError(
        MapError::Unknown, "Creating eBPF map failed", exp_bpf.takeError());
  }
  return exp_bpf.take();
}

ExpectedSuccess<MapError> mapUpdateElement(const int fd,
                                           void const* key,
                                           void const* value,
                                           unsigned long long flags) {
  union bpf_attr attr;
  memset(&attr, 0, sizeof(union bpf_attr));
  attr.map_fd = fd;
  attr.key = reinterpret_cast<std::uint64_t>(key);
  attr.value = reinterpret_cast<std::uint64_t>(value);
  attr.flags = flags;
  auto exp_bpf = syscall(BPF_MAP_UPDATE_ELEM, &attr);
  if (exp_bpf.isError()) {
    return createError(MapError::Unknown,
                       "Updating value in eBPF map failed",
                       exp_bpf.takeError());
  }
  return Success{};
}

ExpectedSuccess<MapError> mapLookupElement(const int fd,
                                           void const* key,
                                           void* value) {
  union bpf_attr attr;
  memset(&attr, 0, sizeof(union bpf_attr));
  attr.map_fd = fd;
  attr.key = reinterpret_cast<std::uint64_t>(key);
  attr.value = reinterpret_cast<std::uint64_t>(value);
  auto exp_bpf = syscall(BPF_MAP_LOOKUP_ELEM, &attr);
  if (exp_bpf.isError()) {
    if (exp_bpf.getErrorCode() == PosixError::NOENT) {
      return createError(MapError::NoSuchKey,
                         "No such key in the eBPF map",
                         exp_bpf.takeError());
    }
    return createError(
        MapError::Unknown, "Look up in eBPF map failed", exp_bpf.takeError());
  }
  return Success{};
}

ExpectedSuccess<MapError> mapDeleteElement(const int fd, void const* key) {
  union bpf_attr attr;
  memset(&attr, 0, sizeof(union bpf_attr));
  attr.map_fd = fd;
  attr.key = reinterpret_cast<std::uint64_t>(key);
  auto exp_bpf = syscall(BPF_MAP_DELETE_ELEM, &attr);
  if (exp_bpf.isError()) {
    if (exp_bpf.getErrorCode() == PosixError::NOENT) {
      return createError(MapError::NoSuchKey,
                         "No such key in the eBPF map",
                         exp_bpf.takeError());
    }
    return createError(MapError::Unknown,
                       "Deleting element from eBPF map failed",
                       exp_bpf.takeError());
  }
  return Success{};
}

} // namespace impl

} // namespace ebpf
} // namespace osquery
