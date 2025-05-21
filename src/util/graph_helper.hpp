#ifndef COMPILER_BITSET_H
#define COMPILER_BITSET_H

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(__popcnt64)
inline int popcount64(uint64_t x) {
  return __popcnt64(x);
}
#else
inline int popcount64(uint64_t x) {
  return __builtin_popcountll(x);
}
#endif

template <typename Container> class IndexedView {
public:
  explicit IndexedView(const Container &container) : container(container) {}

  class iterator {
  public:
    using InnerIter = typename Container::const_iterator;

    iterator(size_t index, InnerIter iter) : index(index), iter(iter) {}

    auto operator*() const { return std::pair{index, *iter}; }

    iterator &operator++() {
      ++index;
      ++iter;
      return *this;
    }

    bool operator!=(const iterator &other) const { return iter != other.iter; }

  private:
    size_t index;
    InnerIter iter;
  };

  iterator begin() const { return iterator(0, container.begin()); }

  iterator end() const { return iterator(container.size(), container.end()); }

private:
  const Container &container;
};

class BitSetIterator {
public:
  BitSetIterator(const std::vector<uint64_t> &bits, size_t total_bits,
                 size_t pos = 0)
      : bits(bits), total_bits(total_bits), pos(pos) {
    advance_to_next();
  }

  size_t operator*() const { return pos; }

  BitSetIterator &operator++() {
    ++pos;
    advance_to_next();
    return *this;
  }

  bool operator!=(const BitSetIterator &other) const {
    return pos != other.pos;
  }

private:
  const std::vector<uint64_t> &bits;
  size_t total_bits;
  size_t pos;

  void advance_to_next() {
    while (pos < total_bits) {
      size_t word = pos / 64;
      size_t bit = pos % 64;
      if (word >= bits.size()) {
        pos = total_bits;
        return;
      }
      if ((bits[word] >> bit) & 1ULL)
        return;
      ++pos;
    }
  }
};

class BitSet {
private:
  std::vector<uint64_t> bits;
  size_t num_bits;

  static constexpr size_t LOG_64 = 6;

  static size_t word_index(size_t pos) { return pos >> LOG_64; }

  static uint64_t bit_mask(size_t pos) {
    return uint64_t(1) << (pos % BITS_PER_WORD);
  }

public:
  static constexpr size_t BITS_PER_WORD = 64;

  explicit BitSet(size_t size) : num_bits(size) {
    size_t num_words = (size + BITS_PER_WORD - 1) / BITS_PER_WORD;
    bits.resize(num_words, 0);
  }

  BitSet(size_t size, bool ones) : num_bits(size) {
    size_t num_words = (size + BITS_PER_WORD - 1) / BITS_PER_WORD;
    bits.resize(num_words, ones ? 0xFFFFFFFFFFFFFFFF : 0);
    if (num_bits%64 > 0) {
      bits[num_words-1] >>= 64 - (num_bits % 64);
    }
  }

  void set(size_t pos) {
    if (pos >= num_bits)
      throw std::out_of_range("set: index out of range");
    bits[word_index(pos)] |= bit_mask(pos);
  }

  void reset(size_t pos) {
    if (pos >= num_bits)
      throw std::out_of_range("reset: index out of range");
    bits[word_index(pos)] &= ~bit_mask(pos);
  }

  void flip(size_t pos) {
    if (pos >= num_bits)
      throw std::out_of_range("flip: index out of range");
    bits[word_index(pos)] ^= bit_mask(pos);
  }

  [[nodiscard]] bool test(size_t pos) const {
    if (pos >= num_bits)
      throw std::out_of_range("test: index out of range");
    return (bits[word_index(pos)] & bit_mask(pos)) != 0;
  }

  [[nodiscard]] size_t size() const { return num_bits; }

  [[nodiscard]] size_t count() const {
    size_t total = 0;
    for (uint64_t word : bits) {
      total += popcount64(word);
    }
    return total;
  }

  [[nodiscard]] std::string to_string() const {
    std::ostringstream oss;
    for (size_t i = num_bits; i-- > 0;) {
      oss << (test(i) ? '1' : '0');
    }
    return oss.str();
  }

  BitSet &operator|=(const BitSet &other) {
    if (num_bits != other.num_bits)
      throw std::invalid_argument("size mismatch");
    for (size_t i = 0; i < bits.size(); ++i) {
      bits[i] |= other.bits[i];
    }
    return *this;
  }

  BitSet &operator&=(const BitSet &other) {
    if (num_bits != other.num_bits)
      throw std::invalid_argument("size mismatch");
    for (size_t i = 0; i < bits.size(); ++i) {
      bits[i] &= other.bits[i];
    }
    return *this;
  }

  BitSet &operator^=(const BitSet &other) {
    if (num_bits != other.num_bits)
      throw std::invalid_argument("size mismatch");
    for (size_t i = 0; i < bits.size(); ++i) {
      bits[i] ^= other.bits[i];
    }
    return *this;
  }

  using iterator = BitSetIterator;
  [[nodiscard]] iterator begin() const { return {bits, num_bits, 0}; }
  [[nodiscard]] iterator end() const { return {bits, num_bits, num_bits}; }
};

class AdjacencyList {
public:
  explicit AdjacencyList(size_t num_nodes)
      : n(num_nodes),
        adjacency(std::vector<BitSet>{num_nodes, BitSet(num_nodes)}) {}

  void add_edge(size_t u, size_t v) {
    if (u >= n || v >= n || u == v)
      return;
    adjacency[u].set(v);
    adjacency[v].set(u);
  }

  void add_clique(std::unordered_set<size_t> &clique) {
    size_t s = clique.size();
    std::vector<size_t> c(s, 0);
    size_t count = 0;
    for (const auto item : clique) {
      c[count++] = item;
    }
    for (int i = 0; i < s; i++) {
      for (int j = i + 1; j < s; j++) {
        add_edge(c[i], c[j]);
      }
    }
  }

  [[nodiscard]] size_t get_size() const { return n; }

  [[nodiscard]] bool has_edge(size_t u, size_t v) const {
    if (u >= n || v >= n)
      return false;
    return adjacency[u].test(v);
  }

  BitSet &neighbors(size_t u) {
    if (u >= n)
      throw std::out_of_range("Invalid node ID");
    return adjacency[u];
  }

  void remove_edge(size_t u, size_t v) {
    if (u >= n || v >= n)
      return;
    adjacency[u].reset(v);
    adjacency[v].reset(u);
  }

  void clear_node(size_t u) {
    if (u >= n)
      return;
    for (size_t v = 0; v < n; ++v) {
      if (adjacency[u].test(v)) {
        adjacency[v].reset(u);
      }
    }
    adjacency[u] ^= adjacency[u]; // clear all
  }

  [[nodiscard]] size_t size() const { return n; }

  auto smart() const { return IndexedView(adjacency); }

  auto begin() const { return adjacency.begin(); }
  auto end() const { return adjacency.end(); }

  auto begin() { return adjacency.begin(); }
  auto end() { return adjacency.end(); }

  const BitSet &operator[](size_t i) const { return adjacency[i]; }
  BitSet &operator[](size_t i) { return adjacency[i]; }

private:
  size_t n;
  std::vector<BitSet> adjacency;
};

#endif // COMPILER_BITSET_H
