#include "io.hpp"
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>

const io::SourceFile io::read_file(std::string path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + path);
  }

  // Read the file content into a string without assuming its size
  std::stringstream buffer;
  buffer << file.rdbuf();

  return io::SourceFile{path, buffer.str()};
}
