#include "analysis/liveness.hpp"
#include "analysis/semantics.hpp"
#include "code_gen/instruction_selection.hpp"
#include "code_gen/interference_graph.hpp"
#include "code_gen/register_alloc.hpp"
#include "defs/ast.hpp"
#include "defs/ast_printer.hpp"
#include "io/io.hpp"
#include "ir/ir_builder.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "report/report_builder.hpp"
#include <iostream>
#include <memory>

int main(int argc, char *argv[]) {

  const auto file = io::read_file(argv[1]);
  // std::cout << file.content << std::endl;

  const auto diagnostics = std::make_shared<DiagnosticEmitter>();
  const auto source_manager =
      std::make_shared<SourceManager>(file.content, file.name);
  Lexer lexer{file.name, file.content};
  Parser parser{lexer, diagnostics, source_manager};

  const auto unit{parser.parse_translation_unit()};

  // ClangStylePrintVisitor visitor{};
  // unit->accept(visitor);
  // std::cout << visitor.get_content() << std::endl;

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    system("pause");
    return -1;
  }

  semantic::SemanticVisitor semantic_visitor{diagnostics, source_manager};
  unit->accept(semantic_visitor);

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    system("pause");
    return -1;
  }
  IntermediateRepresentation representation{};
  IRBuilder builder{representation, diagnostics, source_manager};
  unit->accept(builder);

  std::cout << representation.to_string() << std::endl;

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    system("pause");
    return -1;
  }

  Liveness liveness{representation};
  liveness.analyse();
  std::cout << liveness.to_string_block_to_live() << std::endl;

  InterferenceGraph i_graph{liveness.get_42()};
  i_graph.construct();
  // std::cout << i_graph.to_string() << std::endl;
  // std::cout << i_graph.to_dot() << std::endl;

  RegisterAllocation reg_alloc{i_graph};
  reg_alloc.color();

  InstructionSelector selector{reg_alloc.get_result()};
  const auto asm_string = selector.generate_function_body(representation);

  std::cout << "Generated Assembly:" << std::endl;
  std::cout << asm_string << std::endl;
  std::cout << "Writing file" << std::endl;
  io::write_file("🤣.s", asm_string);
  system(std::format("gcc 🤣.s -o a.out", argv[1]).c_str());
  std::remove("🤣.s");
  system("pause");
  return 0;
}
