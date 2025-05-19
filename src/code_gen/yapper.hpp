#ifndef COMPILER_YAPPER_H
#define COMPILER_YAPPER_H

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct MIRRegisterMap {
private:
  inline static size_t id_counter = 0;
  std::unordered_map<std::string, size_t> physical_to_live_id{};
  std::unordered_map<size_t, std::string> live_id_to_physical{};
  std::unordered_map<size_t, size_t> virtual_to_live_id{};
  std::unordered_map<size_t, size_t> live_id_to_virtual{};

public:
  std::vector<size_t> get_physical_live_ids() {
    std::vector<size_t> t{};
    t.reserve(live_id_to_physical.size());
    for (const auto &item : live_id_to_physical) {
      t.push_back(item.first);
    }
    return t;
  }
  size_t from_physical(const std::string &s) {
    if (physical_to_live_id.contains(s)) {
      return physical_to_live_id[s];
    } else {
      physical_to_live_id[s] = ++id_counter;
      live_id_to_physical[id_counter] = s;
      return id_counter;
    }
  }
  size_t from_virtual(size_t s) {
    if (virtual_to_live_id.contains(s)) {
      return virtual_to_live_id[s];
    } else {
      virtual_to_live_id[s] = ++id_counter;
      live_id_to_virtual[id_counter] = s;
      return id_counter;
    }
  }
  std::optional<size_t> virtual_from_live(size_t id) {
    if (live_id_to_virtual.contains(id)) {
      return live_id_to_virtual[id];
    } else {
      return std::nullopt;
    }
  }
  std::optional<std::string> physical_from_live(size_t id) {
    if (live_id_to_physical.contains(id)) {
      return live_id_to_physical[id];
    } else {
      return std::nullopt;
    }
  }
};

#endif // COMPILER_YAPPER_H
