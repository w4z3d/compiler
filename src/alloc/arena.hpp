#ifndef ALLOC_ARENA_H
#define ALLOC_ARENA_H

#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <type_traits>

namespace arena {
class Arena {
public:
  explicit Arena(std::size_t size) : size(size), offset(0) {
    memory = static_cast<char *>(std::malloc(size));
    if (!memory) {
      spdlog::log(spdlog::level::critical, "Failed to allocate arena!");
      throw std::bad_alloc();
    }
  }

  Arena(const Arena &) = delete;
  Arena &operator=(const Arena &) = delete;

  Arena(Arena &&) noexcept = default;
  Arena &operator=(Arena &&) noexcept = default;

  ~Arena() { std::free(memory); }

  template <typename T> T *make(T value) {
    spdlog::log(spdlog::level::info, "Allocating {} in arena", sizeof(value));
    static_assert(std::is_trivially_destructible<T>(),
                  "Type is not destructable");
    void *mem = allocate_raw(sizeof(T), alignof(T));
    const auto ptr = static_cast<T *>(mem);
    *ptr = value;
    return ptr;
  }

  void *allocate_raw(size_t allocation_size,
                     size_t alignment = alignof(std::max_align_t)) {
    std::size_t space = size - offset;
    void *ptr = memory + offset;
    void *aligned_ptr = std::align(alignment, allocation_size, ptr, space);

    // TODO: Make buffer auto grow
    if (!aligned_ptr || space < allocation_size) {
      throw std::bad_alloc();
    }

    offset = size - space + size;
    return aligned_ptr;
  }

private:
  size_t size;
  size_t offset;
  char *memory;
};
} // namespace arena

#endif // !DEBUG
