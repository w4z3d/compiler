#include "analysis/liveness.hpp"
#include "analysis/semantics.hpp"
#include "code_gen/interference_graph.hpp"
#include "code_gen/register_alloc.hpp"
#include "code_gen/target/target.hpp"
#include "code_gen/target/target_builder.hpp"
#include "code_gen/target/x86/X86.hpp"
#include "code_gen/target/x86/generator.hpp"
#include "defs/ast.hpp"
#include "defs/ast_printer.hpp"
#include "io/io.hpp"
#include "ir/ir_builder.hpp"
#include "lexer/lexer.hpp"
#include "mir/mir_generator.hpp"
#include "opt/mir/mir_optimization_pass.hpp"
#include "opt/mir/peephole_pass.hpp"
#include "parser/parser.hpp"
#include "report/report_builder.hpp"
#include <iostream>
#include <memory>

int main(int argc, char *argv[]) {
  const auto target =
      create_compiler_target<X86_64Target>(CompilerTarget::X86_64);

  const auto file = io::read_file(argv[1]);
  // std::cout << file.content << std::endl;

  const auto diagnostics = std::make_shared<DiagnosticEmitter>();
  const auto source_manager =
      std::make_shared<SourceManager>(file.content, file.name);
  auto *lexer = new Lexer{file.name, file.content};
  auto *parser = new Parser{*lexer, diagnostics, source_manager};

  const auto unit{parser->parse_translation_unit()};

  //ClangStylePrintVisitor visitor{};
  //unit->accept(visitor);
  //std::cout << visitor.get_content() << std::endl;

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 42;
  }
  semantic::SemanticVisitor semantic_visitor{diagnostics, source_manager};
  unit->accept(semantic_visitor);
  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 7;
  }
  IntermediateRepresentation representation{};
  IRBuilder builder{representation, diagnostics, source_manager};
  unit->accept(builder);

  //std::cout << representation.to_string() << std::endl;

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    system("pause");
    return 7;
  }

  delete parser;
  delete lexer;

  mir::MIRProgram program{};
  MIRGenerator mir_generator{representation, program};
  mir_generator.generate();
  //std::cout << mir::to_string(program) << std::endl;

  MIRRegisterMap m{};
  Liveness liveness{program, m};
  liveness.analyse();
  std::cout << liveness.to_string_block_to_live() << std::endl;

  RegisterAllocation reg_alloc{liveness, m,
                               program.get_functions().begin()->second, target};
  reg_alloc.allocate();
  //std::cout << mir::to_string(program) << std::endl;

  /*
    InterferenceGraph i_graph{liveness.get_42()};
    i_graph.construct();
    // std::cout << i_graph.to_string() << std::endl;
    // std::cout << i_graph.to_dot() << std::endl;

    RegisterAllocation reg_alloc{i_graph};
    reg_alloc.color();

    InstructionSelector selector{reg_alloc.get_result(), diagnostics};
    const auto asm_string = selector.generate_function_body(representation);
     */

  // Opt passes
  MIROptPhase mir_opt_phase{{new MIRPeepholePass{}}};
  mir_opt_phase.perform_passes(program);
  std::cout << mir::to_string(program) << std::endl;

  X86Generator gen{};
  const auto asm_string = gen.generate_program(program);

  std::cout << "Generated Assembly:" << std::endl;
  std::cout << asm_string << std::endl;
  std::cout << "Writing file" << std::endl;
  io::write_file("🤣.s", asm_string);
  system(std::format("gcc 🤣.s -o {}", argv[2]).c_str());
  std::remove("🤣.s");
  return 0;
}
