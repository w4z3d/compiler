#ifndef IO_H
#define IO_H

#include <string>

namespace io {
struct SourceFile {
  std::string name;
  std::string content;
};

SourceFile read_file(std::string &path);

} // namespace io

#endif // !IO_H
