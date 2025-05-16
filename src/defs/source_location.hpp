#ifndef DEFS_SOURCE_LOCATION_H
#define DEFS_SOURCE_LOCATION_H

#include <string_view>
#include <tuple>
struct SourceLocation {
  std::string_view file_name;
  std::tuple<int, int> begin;
  std::tuple<int, int> end;
  SourceLocation(std::string_view file_name = "",
                 std::tuple<int, int> begin = {0, 0},
                 std::tuple<int, int> end = {0, 0})
      : file_name(file_name), begin(begin), end(end) {}

public:
  [[nodiscard]] int start_line() const { return std::get<0>(begin); }
  [[nodiscard]] int end_line() const { return std::get<0>(end); }
  [[nodiscard]] int start_col() const { return std::get<1>(begin); }
  [[nodiscard]] int end_col() const { return std::get<1>(end); }
};

#endif // !DEFS_SOURCE_LOCATION_H
