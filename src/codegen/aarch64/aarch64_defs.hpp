// codegen/aarch64/aarch64_defs.hpp
#ifndef SRC_CODEGEN_AARCH64_DEFS_HPP
#define SRC_CODEGEN_AARCH64_DEFS_HPP

#include "../../lir/lir.hpp"
#include "../target.hpp"

namespace aarch64 {

constexpr lir::Register x0 = lir::Register::preg(0);
constexpr lir::Register x1 = lir::Register::preg(1);
constexpr lir::Register x2 = lir::Register::preg(2);
constexpr lir::Register x3 = lir::Register::preg(3);
constexpr lir::Register x4 = lir::Register::preg(4);
constexpr lir::Register x5 = lir::Register::preg(5);
constexpr lir::Register x6 = lir::Register::preg(6);
constexpr lir::Register x7 = lir::Register::preg(7);
constexpr lir::Register x8 = lir::Register::preg(8);
constexpr lir::Register x9 = lir::Register::preg(9);
constexpr lir::Register x10 = lir::Register::preg(10);
constexpr lir::Register x11 = lir::Register::preg(11);
constexpr lir::Register x12 = lir::Register::preg(12);
constexpr lir::Register x13 = lir::Register::preg(13);
constexpr lir::Register x14 = lir::Register::preg(14);
constexpr lir::Register x15 = lir::Register::preg(15);
constexpr lir::Register x16 = lir::Register::preg(16);
constexpr lir::Register x17 = lir::Register::preg(17);
constexpr lir::Register x18 = lir::Register::preg(18);
constexpr lir::Register x19 = lir::Register::preg(19);
constexpr lir::Register x20 = lir::Register::preg(20);
constexpr lir::Register x21 = lir::Register::preg(21);
constexpr lir::Register x22 = lir::Register::preg(22);
constexpr lir::Register x23 = lir::Register::preg(23);
constexpr lir::Register x24 = lir::Register::preg(24);
constexpr lir::Register x25 = lir::Register::preg(25);
constexpr lir::Register x26 = lir::Register::preg(26);
constexpr lir::Register x27 = lir::Register::preg(27);
constexpr lir::Register x28 = lir::Register::preg(28);
constexpr lir::Register x29 = lir::Register::preg(29);
constexpr lir::Register x30 = lir::Register::preg(30);
constexpr lir::Register sp = lir::Register::preg(31);
constexpr lir::Register xzr = lir::Register::preg(32);

constexpr lir::Register fp = x29;
constexpr lir::Register lr = x30;

constexpr lir::Register arg_regs[] = {x0, x1, x2, x3, x4, x5, x6, x7};

constexpr lir::Register caller_saved[] = {x0,  x1,  x2,  x3,  x4,  x5,  x6,
                                          x7,  x8,  x9,  x10, x11, x12, x13,
                                          x14, x15, x16, x17, x18};

constexpr lir::Register callee_saved[] = {x19, x20, x21, x22, x23,
                                          x24, x25, x26, x27, x28};

constexpr lir::Register allocatable[] = {
    x0,  x1,  x2,  x3,  x4,  x5,  x6,  x7,  x8,  x9,  x10, x11, x12,
    x13, x14, x15, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28};

inline const TargetInfo target = {
    arg_regs, 8, x0, caller_saved, 19, callee_saved, 10, allocatable, 26};

} // namespace aarch64

#endif
