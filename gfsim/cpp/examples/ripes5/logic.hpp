#pragma once
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace ripes5 {
using Word = std::uint32_t;
enum Op : std::uint8_t {
    INVALID,
    ADD,
    ADDI,
    SUB,
    AND,
    OR,
    XOR,
    SLT,
    LUI,
    LW,
    SW,
    BEQ,
    BNE,
    JAL,
    JALR,
    HALT
};
struct Instruction {
    Op op{};
    Word rd{}, rs1{}, rs2{}, immediate{};
};
inline Word sext(Word value, unsigned width) {
    const Word sign = Word{1} << (width - 1);
    return (value ^ sign) - sign;
}
inline Instruction decode(Word word) {
    const Word opcode = word & 127, f3 = (word >> 12) & 7, f7 = word >> 25;
    Instruction i{INVALID, (word >> 7) & 31, (word >> 15) & 31, (word >> 20) & 31, 0};
    if (opcode == 0x33) {
        if (f3 == 0 && f7 == 0)
            i.op = ADD;
        if (f3 == 0 && f7 == 32)
            i.op = SUB;
        if (f3 == 7 && f7 == 0)
            i.op = AND;
        if (f3 == 6 && f7 == 0)
            i.op = OR;
        if (f3 == 4 && f7 == 0)
            i.op = XOR;
        if (f3 == 2 && f7 == 0)
            i.op = SLT;
    } else if (opcode == 0x13 || opcode == 0x03 || opcode == 0x67) {
        if (opcode == 0x13 && f3 == 0)
            i.op = ADDI;
        if (opcode == 0x03 && f3 == 2)
            i.op = LW;
        if (opcode == 0x67 && f3 == 0)
            i.op = JALR;
        i.immediate = sext(word >> 20, 12);
        i.rs2 = 0;
    } else if (opcode == 0x37) {
        i.op = LUI;
        i.immediate = word & 0xfffff000U;
        i.rs1 = i.rs2 = 0;
    } else if (opcode == 0x23 && f3 == 2) {
        i.op = SW;
        i.immediate = sext(((word >> 25) << 5) | ((word >> 7) & 31), 12);
        i.rd = 0;
    } else if (opcode == 0x63 && (f3 == 0 || f3 == 1)) {
        Word bits = ((word >> 31) << 12) | (((word >> 7) & 1) << 11);
        bits |= (((word >> 25) & 63) << 5) | (((word >> 8) & 15) << 1);
        i.op = f3 == 0 ? BEQ : BNE;
        i.immediate = sext(bits, 13);
        i.rd = 0;
    } else if (opcode == 0x6f) {
        Word bits = ((word >> 31) << 20) | (((word >> 12) & 255) << 12);
        bits |= (((word >> 20) & 1) << 11) | (((word >> 21) & 1023) << 1);
        i.op = JAL;
        i.immediate = sext(bits, 21);
        i.rs1 = i.rs2 = 0;
    } else if (word == 0x00100073) {
        i.op = HALT;
        i.rd = i.rs1 = i.rs2 = 0;
    }
    return i;
}
struct Slot {
    bool valid{};
    Word pc{}, word{}, left{}, right{}, result{}, value{};
    bool stalled{};
    bool operator==(const Slot &) const = default;
};
struct ExResult {
    Slot next_slot;
    bool redirect{};
    Word forward_a{}, forward_b{};
    bool operator==(const ExResult &) const = default;
};
struct Event {
    std::uint64_t sequence{};
    Word pc{}, word{}, rd{}, value{};
    bool operator==(const Event &) const = default;
};
struct Store {
    std::uint64_t sequence{};
    Word address{}, value{};
    bool operator==(const Store &) const = default;
};
inline Word writer(const Slot &slot) {
    const auto ins = decode(slot.word);
    switch (ins.op) {
    case ADD:
    case ADDI:
    case SUB:
    case AND:
    case OR:
    case XOR:
    case SLT:
    case LUI:
    case LW:
    case JAL:
    case JALR:
        return ins.rd;
    default:
        return 0;
    }
}
inline std::pair<Word, Word> forward(Word index, Word original, const Slot &mem, const Slot &wb) {
    if (index && index == writer(mem))
        return {mem.result, 1};
    if (index && index == writer(wb))
        return {wb.value, 2};
    return {original, 0};
}
inline ExResult execute(Slot ex, const Slot &mem, const Slot &wb) {
    const auto ins = decode(ex.word);
    const auto [a, fa] = forward(ins.rs1, ex.left, mem, wb);
    const auto [b, fb] = forward(ins.rs2, ex.right, mem, wb);
    const bool redirect =
        ins.op == JAL || ins.op == JALR || (ins.op == BEQ && a == b) || (ins.op == BNE && a != b);
    Word result{};
    switch (ins.op) {
    case ADD:
        result = a + b;
        break;
    case ADDI:
    case LW:
    case SW:
    case JALR:
        result = a + ins.immediate;
        break;
    case SUB:
        result = a - b;
        break;
    case AND:
        result = a & b;
        break;
    case OR:
        result = a | b;
        break;
    case XOR:
        result = a ^ b;
        break;
    case SLT:
        result = (a ^ 0x80000000U) < (b ^ 0x80000000U);
        break;
    case LUI:
        result = ins.immediate;
        break;
    case BEQ:
    case BNE:
    case JAL:
        result = ex.pc + ins.immediate;
        break;
    case INVALID:
        break;
    default:
        throw std::invalid_argument("unsupported instruction in execute");
    }
    ex.result = result;
    ex.right = b;
    return {ex, redirect, fa, fb};
}
inline bool loadUseStall(const Slot &id, const Slot &ex) {
    const auto dec = decode(id.word), ins = decode(ex.word);
    return ins.op == LW && ins.rd && (ins.rd == dec.rs1 || ins.rd == dec.rs2);
}
} // namespace ripes5
