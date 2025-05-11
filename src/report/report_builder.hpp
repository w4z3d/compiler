#ifndef REPORT_ERROR_REPORT_H
#define REPORT_ERROR_REPORT_H

#include <string_view>
#include <vector>

namespace report {

class Error {
protected:
  enum class Kind {
    ParserError,
    SemanticError,
    IrError,
    CodeGenError,
  };

private:
  std::string_view error_message;
  Kind kind;

public:
  explicit Error(std::string_view error_message, Kind kind)
      : error_message(error_message), kind(kind) {}
  virtual ~Error() = default;
  [[nodiscard]] virtual std::string_view to_string() noexcept = 0;

  Error(const Error &) = default;
};

class ParserError : public Error {
public:
  explicit ParserError(std::string_view error_message,
                       std::string_view source_file)
      : Error(error_message, Kind::ParserError) {}

  [[nodiscard]] std::string_view to_string() noexcept override;
};

class ReportBuilder {
private:
  std::vector<Error> errors{};

public:
  [[nodiscard]] bool has_errors() const { return errors.empty(); }

  [[nodiscard]] std::string_view build_report() const;

  template <typename ErrorType> void add_error(const ErrorType &error) {
    errors.push_back(error);
  }
};
} // namespace report
#endif // !REPORT_ERROR_REPORT_H
