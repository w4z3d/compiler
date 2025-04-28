#include "io.hpp"
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>

const io::SourceFile io::read_file(std::string path) {
  std::ifstream file(path, std::ios::in);
  const auto size = std::filesystem::file_size(path);
  std::string buffer(size, '\0');
  file.read(buffer.data(), size);
  return io::SourceFile{path, buffer};
}
