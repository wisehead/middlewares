//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
#pragma once

#include <algorithm>
#include "rport/port.h"

namespace rocksutil {
//--
inline size_t TruncateToPageBoundary(size_t page_size, size_t s) {
  s -= (s & (page_size - 1));
  assert((s % page_size) == 0);
  return s;
}
//--
inline size_t Roundup(size_t x, size_t y) {
  return ((x + y - 1) / y) * y;
}
//--
// This class is to manage an aligned user
// allocated buffer for unbuffered I/O purposes
// though can be used for any purpose.
class AlignedBuffer {
  size_t alignment_;
  std::unique_ptr<char[]> buf_;
  size_t capacity_;
  size_t cursize_;
  char* bufstart_;

public:
  AlignedBuffer()
    : alignment_(),
      capacity_(0),
      cursize_(0),
      bufstart_(nullptr) {
  }

  AlignedBuffer(AlignedBuffer&& o) ROCKSUTIL_NOEXCEPT {
    *this = std::move(o);
  }

  AlignedBuffer& operator=(AlignedBuffer&& o) ROCKSUTIL_NOEXCEPT {
    alignment_ = std::move(o.alignment_);
    buf_ = std::move(o.buf_);
    capacity_ = std::move(o.capacity_);
    cursize_ = std::move(o.cursize_);
    bufstart_ = std::move(o.bufstart_);
    return *this;
  }

  AlignedBuffer(const AlignedBuffer&) = delete;

  AlignedBuffer& operator=(const AlignedBuffer&) = delete;
  //--
  size_t Alignment() const {
    return alignment_;
  }

  size_t Capacity() const {
    return capacity_;
  }

  size_t CurrentSize() const {
    return cursize_;
  }
  //--
  const char* BufferStart() const {
    return bufstart_;
  }

  void Clear() {
    cursize_ = 0;
  }

  void Alignment(size_t alignment) {
    assert(alignment > 0);
    assert((alignment & (alignment - 1)) == 0);
    alignment_ = alignment;
  }
  //--
  // Allocates a new buffer and sets bufstart_ to the aligned first byte
  void AllocateNewBuffer(size_t requestedCapacity) {

    assert(alignment_ > 0);
    assert((alignment_ & (alignment_ - 1)) == 0);

    size_t size = Roundup(requestedCapacity, alignment_);
    buf_.reset(new char[size + alignment_]);

    char* p = buf_.get();
    bufstart_ = reinterpret_cast<char*>(
      (reinterpret_cast<uintptr_t>(p)+(alignment_ - 1)) &
      ~static_cast<uintptr_t>(alignment_ - 1));
    capacity_ = size;
    cursize_ = 0;
  }
  // Used for write
  //--// Returns the number of bytes appended
  size_t Append(const char* src, size_t append_size) {
    size_t buffer_remaining = capacity_ - cursize_;
    size_t to_copy = std::min(append_size, buffer_remaining);

    if (to_copy > 0) {
      memcpy(bufstart_ + cursize_, src, to_copy);
      cursize_ += to_copy;
    }
    return to_copy;
  }

  size_t Read(char* dest, size_t offset, size_t read_size) const {
    assert(offset < cursize_);
    size_t to_read = std::min(cursize_ - offset, read_size);
    if (to_read > 0) {
      memcpy(dest, bufstart_ + offset, to_read);
    }
    return to_read;
  }
  //--
  /// Pad to alignment
  void PadToAlignmentWith(int padding) {
    size_t total_size = Roundup(cursize_, alignment_);
    size_t pad_size = total_size - cursize_;

    if (pad_size > 0) {
      assert((pad_size + cursize_) <= capacity_);
      memset(bufstart_ + cursize_, padding, pad_size);
      cursize_ += pad_size;
    }
  }
  //--
  // After a partial flush move the tail to the beginning of the buffer
  void RefitTail(size_t tail_offset, size_t tail_size) {
    if (tail_size > 0) {
      memmove(bufstart_, bufstart_ + tail_offset, tail_size);
    }
    cursize_ = tail_size;
  }

  // Returns place to start writing
  char* Destination() {
    return bufstart_ + cursize_;
  }

  void Size(size_t cursize) {
    cursize_ = cursize;
  }
};
}
