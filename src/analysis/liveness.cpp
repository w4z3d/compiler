#include "liveness.hpp"

#include <ranges>

template <typename T>
std::unordered_set<T> difference(std::unordered_set<T> a, T b) {
  std::unordered_set<T> result;
  a.erase(b);
  return a;
}

template <typename T>
void union_sets(std::unordered_set<T> &a, const std::unordered_set<T> &b) {
  for (const auto &elem : b) {
    a.insert(elem);
  }
}

void Liveness::analyse() {
  for (const auto &cfg : intermediateRepresentation.get_cfgs()) {
    analyse_cfg(cfg);
  }
}

void Liveness::analyse_cfg(const CFG &cfg) {
  // soweit reicht es ja nur den ersten block anzuschauen, da es nur einen block
  // gibt...
  auto current_block = cfg.get_entry_block();
  std::vector<std::unordered_set<Var>> lives_per_line{};
  std::unordered_set<Var> prev_line{};
  for (const auto &iri :
       std::ranges::reverse_view(current_block->get_instructions())) {
    lives_per_line.push_back(iri.get_used());
    if (iri.get_result().has_value()) {
      union_sets(lives_per_line.back(),
                 difference(prev_line, iri.get_result().value()));
    } else {
      union_sets(lives_per_line.back(), prev_line);
    }
    prev_line = lives_per_line.back();
  }
  block_to_live.emplace(current_block->get_id(), lives_per_line);
}
