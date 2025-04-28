#ifndef IO_H
#define IO_H

#include <string>
#include <string_view>
namespace io {
struct SourceFile {
  std::string name;
  std::string content;
};

const SourceFile read_file(std::string path);

} // namespace io

#endif // !IO_H
