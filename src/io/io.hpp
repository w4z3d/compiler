#ifndef IO_H
#define IO_H

#include <string>

namespace io {
struct SourceFile {
  std::string name;
  std::string content;
};

SourceFile read_file(const std::string &path);
bool write_file(const std::string &path, const std::string &content);

} // namespace io

#endif // !IO_H
