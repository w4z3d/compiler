#ifndef REPORT_ERROR_REPORT_H
#define REPORT_ERROR_REPORT_H
// SourceManager class for accessing source code
#include "../defs/ast.hpp"
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class SourceManager {
private:
  std::vector<std::string> source_lines;
  std::string filename;

public:
  SourceManager(const std::string &source, std::string filename)
      : filename(std::move(filename)) {
    // Split source into lines
    size_t pos = 0;
    size_t prev = 0;
    while ((pos = source.find('\n', prev)) != std::string::npos) {
      source_lines.push_back(source.substr(prev, pos - prev));
      prev = pos + 1;
    }
    source_lines.push_back(source.substr(prev));
  }

  [[nodiscard]] std::string get_line(int line_num) const {
    if (line_num > 0 && line_num <= static_cast<int>(source_lines.size())) {
      return source_lines[line_num - 1];
    }
    return "";
  }

  [[nodiscard]] std::string get_snippet(const SourceLocation &loc) const {
    if (loc.start_line() == loc.end_line()) {
      std::string line = get_line(loc.start_line());
      if (!line.empty()) {
        return line;
      }
    }
    // For multi-line snippets, return the first line
    return get_line(loc.start_line());
  }

  [[nodiscard]] std::string get_filename() const { return filename; }
};

enum class DiagnosticSeverity { Error, Warning, Note, Hint };

// Color codes for terminal output
namespace Color {
const std::string Reset = "\033[0m";
const std::string Red = "\033[31m";
const std::string Green = "\033[32m";
const std::string Yellow = "\033[33m";
const std::string Blue = "\033[34m";
const std::string Magenta = "\033[35m";
const std::string Cyan = "\033[36m";
const std::string Bold = "\033[1m";
} // namespace Color

struct Diagnostic {
  DiagnosticSeverity severity;
  SourceLocation location;
  std::string message;
  std::optional<std::string> code_snippet;
  std::optional<std::string> fix_suggestion;

  [[nodiscard]] std::string severity_to_string() const {
    switch (severity) {
    case DiagnosticSeverity::Error:
      return "error";
    case DiagnosticSeverity::Warning:
      return "warning";
    case DiagnosticSeverity::Note:
      return "note";
    case DiagnosticSeverity::Hint:
      return "hint";
    }
    return "unknown";
  }

  [[nodiscard]] std::string severity_color() const {
    switch (severity) {
    case DiagnosticSeverity::Error:
      return Color::Red;
    case DiagnosticSeverity::Warning:
      return Color::Yellow;
    case DiagnosticSeverity::Note:
      return Color::Blue;
    case DiagnosticSeverity::Hint:
      return Color::Green;
    }
    return "";
  }

  [[nodiscard]] std::string formatted_message() const {
    std::string result;

    // Location
    result += std::format("{}:{}:{}: ", location.file_name,
                          location.start_line(), location.start_col());

    // Severity with color
    result += severity_color() + Color::Bold + severity_to_string() +
              Color::Reset + ": ";

    // Main message
    result += message + "\n";

    // Code snippet if available
    if (code_snippet) {
      result += "  " + *code_snippet + "\n";

      // Generate pointer line to indicate the error position
      int start_col = location.start_col();
      std::string pointer_line = std::string(start_col - 1, ' ');

      int length = location.end_col() - location.start_col();
      if (length <= 0)
        length = 1;

      pointer_line +=
          severity_color() + std::string(length, '^') + Color::Reset;
      result += "  " + pointer_line + "\n";
    }

    // Fix suggestion if available
    if (fix_suggestion) {
      result +=
          Color::Green + "  note: " + *fix_suggestion + Color::Reset + "\n";
    }

    return result;
  }
};

class DiagnosticEmitter {
private:
  std::vector<Diagnostic> diagnostics;
  bool continue_on_error = true;

public:
  void emit_error(const SourceLocation &loc, const std::string &message) {
    diagnostics.push_back(
        {DiagnosticSeverity::Error, loc, message, std::nullopt, std::nullopt});
  }

  void emit_warning(const SourceLocation &loc, const std::string &message) {
    diagnostics.push_back({DiagnosticSeverity::Warning, loc, message,
                           std::nullopt, std::nullopt});
  }

  void emit_note(const SourceLocation &loc, const std::string &message) {
    diagnostics.push_back(
        {DiagnosticSeverity::Note, loc, message, std::nullopt, std::nullopt});
  }

  void emit_hint(const SourceLocation &loc, const std::string &message) {
    diagnostics.push_back(
        {DiagnosticSeverity::Hint, loc, message, std::nullopt, std::nullopt});
  }

  void suggest_fix(const std::string &suggestion) {
    if (!diagnostics.empty()) {
      diagnostics.back().fix_suggestion = suggestion;
    }
  }

  void add_source_context(const std::string &code) {
    if (!diagnostics.empty()) {
      diagnostics.back().code_snippet = code;
    }
  }

  [[nodiscard]] const std::vector<Diagnostic> &get_diagnostics() const {
    return diagnostics;
  }

  [[nodiscard]] bool has_errors() const {
    return std::any_of(diagnostics.begin(), diagnostics.end(),
                       [](const Diagnostic &d) {
                         return d.severity == DiagnosticSeverity::Error;
                       });
  }

  void print_all(std::ostream &out = std::cerr) const {
    out << std::endl;
    for (const auto &diag : diagnostics) {
      out << diag.formatted_message() << std::endl;
    }

    // Print summary
    size_t error_count = std::count_if(
        diagnostics.begin(), diagnostics.end(), [](const Diagnostic &d) {
          return d.severity == DiagnosticSeverity::Error;
        });

    size_t warning_count = std::count_if(
        diagnostics.begin(), diagnostics.end(), [](const Diagnostic &d) {
          return d.severity == DiagnosticSeverity::Warning;
        });

    if (error_count > 0 || warning_count > 0) {
      out << Color::Bold;
      if (error_count > 0) {
        out << Color::Red << error_count << " error"
            << (error_count > 1 ? "s" : "");
      }

      if (error_count > 0 && warning_count > 0) {
        out << Color::Reset << " and ";
      }

      if (warning_count > 0) {
        out << Color::Yellow << warning_count << " warning"
            << (warning_count > 1 ? "s" : "");
      }

      out << " generated." << Color::Reset << "\n";
    }
  }

  // Set whether to continue parsing after errors
  void set_continue_on_error(bool value) { continue_on_error = value; }

  [[nodiscard]] bool should_continue() const {
    return continue_on_error || !has_errors();
  }

  // Clear all diagnostics
  void clear() { diagnostics.clear(); }
};

#endif
