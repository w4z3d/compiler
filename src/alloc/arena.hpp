#ifndef ALLOC_ARENA_H
#define ALLOC_ARENA_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace arena {
class Arena {
private:
  struct Block {
    std::unique_ptr<std::uint8_t[]> data;
    std::size_t size;
    std::size_t used;

    explicit Block(std::size_t block_size) : size(block_size), used(0) {
      data = std::make_unique<std::uint8_t[]>(block_size);
    }
  };

  std::vector<Block> blocks_;
  std::size_t block_size_;
  std::size_t alignment_;

  [[nodiscard]] inline std::size_t align(std::size_t n) const noexcept {
    return (n + alignment_ - 1) & ~(alignment_ - 1);
  }

public:
  explicit Arena(std::size_t block_size = 4096, size_t alignment = 8)
      : block_size_(block_size), alignment_(alignment) {
    blocks_.emplace_back(block_size_);
  }

  void *allocate(std::size_t size) {
    if (size == 0)
      return nullptr;

    // Allocate a dedicated block if the size exceedes the standard blocksize
    // (So we dont end up in a loop of generating blocks)
    if (size > block_size_) {
      blocks_.emplace_back(size);
      auto &block = blocks_.back();
      block.used = size;
      return block.data.get();
    }

    auto &current_block = blocks_.back();
    size_t align_used = align(current_block.used);

    // Check if enough space in current block
    if (align_used + size <= current_block.size) {
      void *ptr = current_block.data.get() + align_used;
      current_block.used = align_used + size;
      return ptr;
    }

    // Not enough space, create new block
    blocks_.emplace_back(block_size_);
    auto &new_block = blocks_.back();
    new_block.used = size;
    return new_block.data.get();
  }

  template <typename T, typename... Args>
  [[nodiscard]] T *create(Args &&...args) {
    void *mem = allocate(sizeof(T));
    if (!mem)
      return nullptr;
    return new (mem) T(std::forward<Args>(args)...);
  }

  // Reset but dont free
  void reset() noexcept {
    if (!blocks_.empty()) {
      // Take ownership
      auto first_block = std::move(blocks_.front());
      blocks_.clear();
      first_block.used = 0;
      // Move ownership
      blocks_.push_back(std::move(first_block));
    }
  }

  void clear() noexcept {
    blocks_.clear();
    blocks_.emplace_back(block_size_);
  }

  [[nodiscard]] std::size_t size() const noexcept {
    std::size_t total = 0;
    for (const auto &block : blocks_) {
      total += block.size;
    }
    return total;
  }

  [[nodiscard]] std::size_t used() const {
    std::size_t total = 0;
    for (const auto &block : blocks_) {
      total += block.used;
    }
    return total;
  }

  ~Arena() = default;

  Arena(const Arena &) = delete;
  Arena &operator=(const Arena &) = delete;
  Arena(Arena &&) = delete;
  Arena &operator=(Arena &&) = delete;
};
} // namespace arena

#endif // !DEBUG
