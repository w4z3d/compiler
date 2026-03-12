#ifndef REPORT_ERROR_REPORT_H
#define REPORT_ERROR_REPORT_H

#include "../defs/source_location.hpp"
#include <algorithm>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class SourceManager {
  std::vector<std::string> source_lines;
  std::string filename;

public:
  SourceManager(const std::string &source, std::string filename)
      : filename(std::move(filename)) {
    size_t pos = 0;
    size_t prev = 0;
    while ((pos = source.find('\n', prev)) != std::string::npos) {
      source_lines.push_back(source.substr(prev, pos - prev));
      prev = pos + 1;
    }
    source_lines.push_back(source.substr(prev));
  }

  [[nodiscard]] std::string get_line(int line_num) const {
    if (line_num > 0 && line_num <= static_cast<int>(source_lines.size()))
      return source_lines[line_num - 1];
    return "";
  }

  [[nodiscard]] std::string get_snippet(const SourceLocation &loc) const {
    return get_line(loc.start_line());
  }

  [[nodiscard]] const std::string &get_filename() const { return filename; }
};

namespace Color {
inline constexpr const char *Reset = "\033[0m";
inline constexpr const char *Red = "\033[31m";
inline constexpr const char *Green = "\033[32m";
inline constexpr const char *Yellow = "\033[33m";
inline constexpr const char *Blue = "\033[34m";
inline constexpr const char *Magenta = "\033[35m";
inline constexpr const char *Cyan = "\033[36m";
inline constexpr const char *Bold = "\033[1m";
inline constexpr const char *Dim = "\033[2m";
} // namespace Color

enum class DiagnosticSeverity { Error, Warning, Note, Hint };

struct SubDiagnostic {
  DiagnosticSeverity severity;
  std::string message;
  std::optional<SourceLocation> location;
  std::optional<std::string> code_snippet;
};

struct Diagnostic {
  DiagnosticSeverity severity;
  SourceLocation location;
  std::string message;
  std::optional<std::string> code_snippet;

  std::vector<SubDiagnostic> notes;
  std::vector<std::string> fix_suggestions;

  [[nodiscard]] static const char *severity_str(DiagnosticSeverity s) {
    switch (s) {
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

  [[nodiscard]] static const char *severity_color(DiagnosticSeverity s) {
    switch (s) {
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

  static std::string format_snippet(const std::string &snippet,
                                    const SourceLocation &loc,
                                    DiagnosticSeverity sev) {
    std::string result;
    int line_num = loc.start_line();

    // Line number gutter
    result += std::format("{}  {} │{} {}\n", Color::Dim, line_num, Color::Reset,
                          snippet);

    // Pointer line
    int start_col = loc.start_col();
    int length = loc.end_col() - loc.start_col();
    if (length <= 0)
      length = 1;

    std::string gutter_pad(std::to_string(line_num).size() + 3, ' ');
    result += std::format("{}│ {}{}{}{}", gutter_pad,
                          std::string(start_col - 1, ' '), severity_color(sev),
                          std::string(length, '^'), Color::Reset);

    return result;
  }

  [[nodiscard]] std::string format() const {
    std::string result;

    result += std::format(
        "{}:{}:{}: {}{}{}{}:{} {}\n", location.file_name, location.start_line(),
        location.start_col(), severity_color(severity), Color::Bold,
        severity_str(severity), Color::Reset, Color::Bold, message);
    result += Color::Reset;

    if (code_snippet) {
      result += format_snippet(*code_snippet, location, severity);
      result += "\n";
    }

    for (auto &fix : fix_suggestions) {
      result += std::format("  {}{}suggestion:{} {}\n", Color::Green,
                            Color::Bold, Color::Reset, fix);
    }

    for (auto &sub : notes) {
      if (sub.location) {
        result += std::format(
            "{}:{}:{}: {}{}{}{}:{} {}\n", sub.location->file_name,
            sub.location->start_line(), sub.location->start_col(),
            severity_color(sub.severity), Color::Bold,
            severity_str(sub.severity), Color::Reset, Color::Bold, sub.message);
        result += Color::Reset;
      } else {
        result += std::format("  {}{}{}{}: {}\n", severity_color(sub.severity),
                              Color::Bold, severity_str(sub.severity),
                              Color::Reset, sub.message);
      }

      if (sub.code_snippet && sub.location) {
        result +=
            format_snippet(*sub.code_snippet, *sub.location, sub.severity);
        result += "\n";
      }
    }

    return result;
  }
};

class DiagnosticEmitter;

class DiagnosticBuilder {
  DiagnosticEmitter &emitter;
  Diagnostic diag;

public:
  DiagnosticBuilder(DiagnosticEmitter &emitter, DiagnosticSeverity sev,
                    SourceLocation loc, std::string message)
      : emitter(emitter) {
    diag.severity = sev;
    diag.location = std::move(loc);
    diag.message = std::move(message);
  }

  DiagnosticBuilder &with_snippet(const std::string &code) {
    diag.code_snippet = code;
    return *this;
  }

  DiagnosticBuilder &with_snippet(const SourceManager &sm) {
    diag.code_snippet = sm.get_line(diag.location.start_line());
    return *this;
  }

  DiagnosticBuilder &suggest(const std::string &fix) {
    diag.fix_suggestions.push_back(fix);
    return *this;
  }

  DiagnosticBuilder &note(const std::string &message) {
    diag.notes.push_back({
        DiagnosticSeverity::Note,
        message,
        std::nullopt,
        std::nullopt,
    });
    return *this;
  }

  DiagnosticBuilder &note(const std::string &message,
                          const SourceLocation &loc) {
    diag.notes.push_back({
        DiagnosticSeverity::Note,
        message,
        loc,
        std::nullopt,
    });
    return *this;
  }

  DiagnosticBuilder &note(const std::string &message, const SourceLocation &loc,
                          const std::string &snippet) {
    diag.notes.push_back({
        DiagnosticSeverity::Note,
        message,
        loc,
        snippet,
    });
    return *this;
  }

  DiagnosticBuilder &note(const std::string &message, const SourceLocation &loc,
                          const SourceManager &sm) {
    diag.notes.push_back({
        DiagnosticSeverity::Note,
        message,
        loc,
        sm.get_line(loc.start_line()),
    });
    return *this;
  }

  DiagnosticBuilder &hint(const std::string &message) {
    diag.notes.push_back({
        DiagnosticSeverity::Hint,
        message,
        std::nullopt,
        std::nullopt,
    });
    return *this;
  }

  void emit();

  ~DiagnosticBuilder() { emit(); }
};

class DiagnosticEmitter {
  friend class DiagnosticBuilder;

  std::vector<Diagnostic> diagnostics;

  void add(Diagnostic diag) { diagnostics.push_back(std::move(diag)); }

public:
  DiagnosticBuilder error(const SourceLocation &loc,
                          const std::string &message) {
    return {*this, DiagnosticSeverity::Error, loc, message};
  }

  DiagnosticBuilder warning(const SourceLocation &loc,
                            const std::string &message) {
    return {*this, DiagnosticSeverity::Warning, loc, message};
  }

  void emit_error(const SourceLocation &loc, const std::string &msg) {
    diagnostics.push_back(
        {DiagnosticSeverity::Error, loc, msg, std::nullopt, {}, {}});
  }

  void emit_warning(const SourceLocation &loc, const std::string &msg) {
    diagnostics.push_back(
        {DiagnosticSeverity::Warning, loc, msg, std::nullopt, {}, {}});
  }

  void suggest_fix(const std::string &suggestion) {
    if (!diagnostics.empty())
      diagnostics.back().fix_suggestions.push_back(suggestion);
  }

  void add_source_context(const std::string &code) {
    if (!diagnostics.empty())
      diagnostics.back().code_snippet = code;
  }

  void emit_note(const SourceLocation &loc, const std::string &msg) {
    if (!diagnostics.empty()) {
      diagnostics.back().notes.push_back(
          {DiagnosticSeverity::Note, msg, loc, std::nullopt});
    }
  }

  void emit_hint(const SourceLocation &loc, const std::string &msg) {
    if (!diagnostics.empty()) {
      diagnostics.back().notes.push_back(
          {DiagnosticSeverity::Hint, msg, loc, std::nullopt});
    }
  }

  [[nodiscard]] bool has_errors() const {
    return std::any_of(diagnostics.begin(), diagnostics.end(),
                       [](const Diagnostic &d) {
                         return d.severity == DiagnosticSeverity::Error;
                       });
  }

  [[nodiscard]] const std::vector<Diagnostic> &get_diagnostics() const {
    return diagnostics;
  }

  void print_all(std::ostream &out = std::cerr) const {
    out << "\n";
    for (auto &diag : diagnostics)
      out << diag.format() << "\n";

    // Summary
    size_t errors =
        std::count_if(diagnostics.begin(), diagnostics.end(), [](auto &d) {
          return d.severity == DiagnosticSeverity::Error;
        });
    size_t warnings =
        std::count_if(diagnostics.begin(), diagnostics.end(), [](auto &d) {
          return d.severity == DiagnosticSeverity::Warning;
        });

    if (errors > 0 || warnings > 0) {
      out << Color::Bold;
      if (errors > 0)
        out << Color::Red << errors << (errors == 1 ? " error" : " errors");
      if (errors > 0 && warnings > 0)
        out << Color::Reset << " and ";
      if (warnings > 0)
        out << Color::Yellow << warnings
            << (warnings == 1 ? " warning" : " warnings");
      out << " generated." << Color::Reset << "\n";
    }
  }

  void clear() { diagnostics.clear(); }
};

inline void DiagnosticBuilder::emit() {
  if (!diag.message.empty()) {
    emitter.add(std::move(diag));
    diag.message.clear(); // prevent double-emit
  }
}

#endif
