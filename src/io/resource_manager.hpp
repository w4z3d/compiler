#ifndef IO_RESOURCE_MANAGER_H
#define IO_RESOURCE_MANAGER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
struct ResourceManager {
private:
  std::unordered_map<std::string, std::string_view> sources;
  std::unordered_map<std::string, std::vector<std::string_view>> lines;

public:
  [[nodiscard]] std::string_view get_source(const std::string &name) const {
    return sources.at(name);
  }

  void add_source_file(const std::string &name, std::string_view source) {
    sources.emplace(name, source);
  }

  [[nodiscard]] const std::vector<std::string_view> &
  get_lines(const std::string &name) const {
    return lines.at(name);
  }

  void split_to_lines(std::string_view name) {
    size_t start = 0;
    size_t end = 0;
    if (!lines[name].empty())
      return;
    lines[name] = {};

    const auto content = get_source(name);
    std::vector<std::string_view> split_lines{};

    while ((end = content.find_first_of("\r\n", start)) !=
           std::string_view::npos) {
      // Add the line
      lines[name].push_back(content.substr(start, end - start));

      // Skip the newline character(s)
      if (content[end] == '\r' && end + 1 < content.size() &&
          content[end + 1] == '\n') {
        start = end + 2; // Skip \r\n
      } else {
        start = end + 1; // Skip \n or \r
      }
    }

    // Add the last line if there's content after the last newline
    if (start < content.size()) {
      lines[name].push_back(content.substr(start));
    }
  }
};

#endif // !IO_RESOURCE_MANAGER_H
