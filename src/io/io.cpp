#include "io.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

io::SourceFile io::read_file(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + path);
  }

  // Read the file content into a string without assuming its size
  std::stringstream buffer;
  buffer << file.rdbuf();

  return io::SourceFile{path, buffer.str()};
}

bool io::write_file(const std::string &path, const std::string &content) {
  std::ofstream file(path);

  if (!file.is_open()) {
    return false;
  }
  file << content;
  file.flush();
  file.close();
  return true;
}
