#ifndef ARGS_HPP
#define ARGS_HPP

#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct CompilerOptions {
  std::string input_file;
  std::string output_file = "output";

  bool emit_hir = false;
  bool emit_lir = false;
  bool emit_asm = false;
  bool dump_ast = false;
  bool dump_interference = false;

  int opt_level = 0; // -O0, -O1, -O2
  bool verbose = false;
  bool baremetal = false;

  enum class Target { DarwinAArch64, LinuxAArch64, Windows_x86_64 };
  Target target = Target::DarwinAArch64;

  enum class EmitFormat { Executable, Assembly, Object };
  EmitFormat emit = EmitFormat::Executable;
};

inline std::optional<CompilerOptions> parse_args(int argc, char *argv[]) {
  CompilerOptions opts;

  if (argc < 2) {
    std::cerr
        << "Usage: " << argv[0] << " <input.c0> [options]\n"
        << "\n"
        << "Options:\n"
        << "  -o <file>        Output file\n"
        << "  -S               Emit assembly only\n"
        << "  -c               Emit object file only\n"
        << "  -O0/-O1/-O2      Optimization level\n"
        << "  --emit-hir       Dump HIR\n"
        << "  --emit-lir       Dump LIR\n"
        << "  --dump-ast       Dump AST\n"
        << "  --dump-ig        Dump interference graph\n"
        << "  --target <t>     DarwinAArch64|LinuxAArch64|Windows_x86_64\n"
        << "  -v, --verbose    Verbose output\n";
    return std::nullopt;
  }

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    // Flags with values
    if (arg == "-o" && i + 1 < argc) {
      opts.output_file = argv[++i];
    } else if (arg == "--target" && i + 1 < argc) {
      std::string t = argv[++i];
      if (t == "DarwinAArch64")
        opts.target = CompilerOptions::Target::DarwinAArch64;
      else if (t == "LinuxAArch64")
        opts.target = CompilerOptions::Target::LinuxAArch64;
      else {
        std::cerr << "Unknown target: " << t << "\n";
        return std::nullopt;
      }
    } else if (arg == "-O0")
      opts.opt_level = 0;
    else if (arg == "-O1")
      opts.opt_level = 1;
    else if (arg == "-O2")
      opts.opt_level = 2;
    else if (arg == "-S")
      opts.emit = CompilerOptions::EmitFormat::Assembly;
    else if (arg == "-c")
      opts.emit = CompilerOptions::EmitFormat::Object;
    else if (arg == "--emit-hir")
      opts.emit_hir = true;
    else if (arg == "--emit-lir")
      opts.emit_lir = true;
    else if (arg == "--emit-asm")
      opts.emit_asm = true;
    else if (arg == "--dump-ast")
      opts.dump_ast = true;
    else if (arg == "--dump-ig")
      opts.dump_interference = true;
    else if (arg == "-v" || arg == "--verbose")
      opts.verbose = true;
    else if (arg[0] != '-') {
      opts.input_file = arg;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      return std::nullopt;
    }
  }

  if (opts.input_file.empty()) {
    std::cerr << "Error: no input file\n";
    return std::nullopt;
  }

  return opts;
}

#endif
