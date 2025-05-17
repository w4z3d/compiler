#ifndef MIR_MIR_GENERATOR
#define MIR_MIR_GENERATOR

#include "../ir/cfg.hpp"
#include "mir.hpp"
struct MIRGenerator {
private:
  IntermediateRepresentation &representation;
  mir::MIRProgram &mir_program;

public:
  explicit MIRGenerator(IntermediateRepresentation &representation,
                        mir::MIRProgram &program)
      : representation(representation), mir_program(program) {}

  void generate();
};

#endif // !MIR_MIR_GENERATOR
