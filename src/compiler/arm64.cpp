// The ARM64 back end: IR to AArch64 machine code with the Windows calling
// convention (integer arguments in x0-x7, doubles in d0-d7 counted apart, the
// rest on the stack, the result in x0 or d0). Like the other back ends it keeps
// every value in its frame slot and uses a few scratch registers: x0-x7 for
// arguments and results, x9-x15 for values, x16 and x17 for addresses.
//
// Every reference inside the image is PC-relative (`adr`), so the image needs
// no relocations. The entry table sits at offset 0, eight bytes per service.
//
// Frame, from the low addresses: the area of outgoing stack arguments, then
// [x29] the caller's x29, [x29+8] the return address, and slot k at
// [x29 + 16 + 8k]. Incoming stack arguments are above the frame.

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "native_image.hpp"

namespace wfc::compiler {

namespace {

using ir::Check;
using ir::Condition;
using ir::Opcode;
using ir::Operand;
using ir::OperandKind;

constexpr std::uint32_t kFp = 29;
constexpr std::uint32_t kSp = 31;
constexpr std::uint32_t kLr = 30;
constexpr std::uint32_t kScratchA = 15;  // address of a distant slot
constexpr std::uint32_t kScratchB = 16;  // values, the service address
constexpr std::uint32_t kScratchC = 17;  // pointers and data addresses

// condition codes
constexpr std::uint32_t kEq = 0x0;
constexpr std::uint32_t kNe = 0x1;
constexpr std::uint32_t kMi = 0x4;
constexpr std::uint32_t kVs = 0x6;
constexpr std::uint32_t kLs = 0x9;
constexpr std::uint32_t kGe = 0xA;
constexpr std::uint32_t kLt = 0xB;
constexpr std::uint32_t kGt = 0xC;
constexpr std::uint32_t kLe = 0xD;

[[nodiscard]] std::uint32_t integer_condition(const Condition condition) {
    switch (condition) {
        case Condition::equal:
            return kEq;
        case Condition::not_equal:
            return kNe;
        case Condition::less:
            return kLt;
        case Condition::less_equal:
            return kLe;
        case Condition::greater:
            return kGt;
        case Condition::greater_equal:
            return kGe;
    }
    return kEq;
}

// After an ordered `fcmp` (a NaN never gets this far).
[[nodiscard]] std::uint32_t float_condition(const Condition condition) {
    switch (condition) {
        case Condition::equal:
            return kEq;
        case Condition::not_equal:
            return kNe;
        case Condition::less:
            return kMi;
        case Condition::less_equal:
            return kLs;
        case Condition::greater:
            return kGt;
        case Condition::greater_equal:
            return kGe;
    }
    return kEq;
}

enum class JumpKind { branch, conditional, compare_zero, compare_nonzero };

struct Fixup final {
    std::size_t at{};  // the instruction
    int label{};
    JumpKind kind{JumpKind::branch};
    std::uint32_t operand{};  // the condition, or the register to test
};

class Emitter final {
public:
    explicit Emitter(const ir::Program& program) : program_(program) {}

    BackEndResult run() {
        layout();
        std::vector<std::size_t> function_starts;
        for (const auto& function : program_.functions) {
            function_starts.push_back(image_.size());
            emit_function(function);
        }
        for (const auto& [at, callee] : call_fixups_) {
            const auto words =
                (static_cast<std::int64_t>(
                     function_starts[static_cast<std::size_t>(callee)]) -
                 static_cast<std::int64_t>(at)) /
                4;
            patch(at, 0x94000000U |
                          (static_cast<std::uint32_t>(words) & 0x03FFFFFFU));
        }
        patch_class_records(image_, program_, string_offsets_, function_starts);
        BackEndResult result;
        result.image.architecture = Architecture::arm64;
        result.image.table_count = static_cast<std::size_t>(ir::Service::count);
        result.image.entry =
            function_starts[static_cast<std::size_t>(program_.entry)];
        result.image.error_flag = flag_offset_;
        result.image.bytes = std::move(image_);
        return result;
    }

private:
    // ----- Layout ----------------------------------------------------------

    void layout() {
        std::size_t at = static_cast<std::size_t>(ir::Service::count) * 8U;
        flag_offset_ = at;  // the pending-error flag
        at += 8U;
        limit_offset_ = at;  // the lowest stack address a call may reach
        at += 8U;
        globals_offset_ = at;
        at += program_.globals.size() * 8U;
        // A literal is an immortal string: a reference count of -1, the
        // length and the bytes (the layout of the runtime's strings).
        for (const auto& text : program_.strings) {
            string_offsets_.push_back(at);
            at += 8U + text.size() + 1U;
            at = (at + 7U) & ~static_cast<std::size_t>(7U);
        }
        at = (at + 15U) & ~static_cast<std::size_t>(15U);
        image_.assign(at, 0);
        for (std::size_t i = 0; i < program_.strings.size(); ++i) {
            const auto base = string_offsets_[i];
            const std::int32_t immortal = -1;
            const auto length =
                static_cast<std::int32_t>(program_.strings[i].size());
            std::memcpy(image_.data() + base, &immortal, 4U);
            std::memcpy(image_.data() + base + 4U, &length, 4U);
            std::memcpy(image_.data() + base + 8U, program_.strings[i].data(),
                        program_.strings[i].size());
        }
    }

    // ----- Instruction helpers ---------------------------------------------

    void emit(const std::uint32_t word) {
        for (int shift = 0; shift < 32; shift += 8) {
            image_.push_back(static_cast<std::uint8_t>(word >> shift));
        }
    }

    void patch(const std::size_t at, const std::uint32_t word) {
        for (std::size_t i = 0; i < 4U; ++i) {
            image_[at + i] = static_cast<std::uint8_t>(word >> (8U * i));
        }
    }

    [[nodiscard]] std::uint32_t word_at(const std::size_t at) const {
        std::uint32_t word = 0;
        for (std::size_t i = 0; i < 4U; ++i) {
            word |= static_cast<std::uint32_t>(image_[at + i]) << (8U * i);
        }
        return word;
    }

    // mov wd/xd, #value: a movz of the low half, a movk for each other half
    // that is not zero
    void move_immediate(const std::uint32_t reg, const std::uint64_t value,
                        const bool wide) {
        const std::uint32_t halves = wide ? 4U : 2U;
        emit((wide ? 0xD2800000U : 0x52800000U) |
             (static_cast<std::uint32_t>(value & 0xFFFFU) << 5) | reg);
        for (std::uint32_t i = 1; i < halves; ++i) {
            const auto part =
                static_cast<std::uint32_t>((value >> (16U * i)) & 0xFFFFU);
            if (part != 0) {
                emit((wide ? 0xF2800000U : 0x72800000U) | (i << 21) |
                     (part << 5) | reg);
            }
        }
    }

    // adr xd, the image offset `target`
    void address_of(const std::uint32_t reg, const std::size_t target) {
        const auto offset = static_cast<std::int64_t>(target) -
                            static_cast<std::int64_t>(image_.size());
        const auto bits = static_cast<std::uint32_t>(offset) & 0x1FFFFFU;
        emit(0x10000000U | ((bits & 3U) << 29) | ((bits >> 2) << 5) | reg);
    }

    // ldr/str of 4 or 8 bytes at [base + offset] (offset >= 0, a multiple of
    // the size when it can be encoded; otherwise through kScratchA)
    enum class Access {
        load32,
        load64,
        store32,
        store64,
        load_double,
        store_double
    };

    void memory(const Access access, const std::uint32_t reg,
                const std::uint32_t base, const std::int64_t offset) {
        std::uint32_t opcode = 0;
        std::uint32_t size = 8;
        switch (access) {
            case Access::load32:
                opcode = 0xB9400000U;
                size = 4;
                break;
            case Access::load64:
                opcode = 0xF9400000U;
                break;
            case Access::store32:
                opcode = 0xB9000000U;
                size = 4;
                break;
            case Access::store64:
                opcode = 0xF9000000U;
                break;
            case Access::load_double:
                opcode = 0xFD400000U;
                break;
            case Access::store_double:
                opcode = 0xFD000000U;
                break;
        }
        if (offset >= 0 && offset % size == 0 && offset / size < 4096) {
            emit(opcode | (static_cast<std::uint32_t>(offset / size) << 10) |
                 (base << 5) | reg);
            return;
        }
        if (base == kSp) {
            // beyond the reach of one instruction: through x29, which is at
            // a known distance from sp
            move_immediate(kScratchA,
                           static_cast<std::uint64_t>(offset - fp_offset_),
                           true);
            emit(0x8B000000U | (kScratchA << 16) | (kFp << 5) | kScratchA);
        } else {
            move_immediate(kScratchA, static_cast<std::uint64_t>(offset), true);
            emit(0x8B000000U | (kScratchA << 16) | (base << 5) | kScratchA);
        }
        emit(opcode | (kScratchA << 5) | reg);
    }

    // Higher slots are at lower addresses (as on the other back ends, which
    // some services rely on to read consecutive slots upward).
    [[nodiscard]] std::int64_t slot_offset(const std::int64_t slot) const {
        return out_ + 8 * (slots_ - 1 - slot);
    }

    [[nodiscard]] std::size_t global_at(const std::int64_t index) const {
        return globals_offset_ + static_cast<std::size_t>(index) * 8U;
    }

    // add xd, xn, #imm (imm any non-negative value)
    void add_immediate(const std::uint32_t dst, const std::uint32_t src,
                       const std::int64_t imm) {
        if (imm >= 0 && imm < 4096) {
            emit(0x91000000U | (static_cast<std::uint32_t>(imm) << 10) |
                 (src << 5) | dst);
        } else {
            move_immediate(kScratchA, static_cast<std::uint64_t>(imm), true);
            if (src == kSp) {
                // add xd, sp, x15 (the extended form reads sp)
                emit(0x8B206000U | (kScratchA << 16) | (src << 5) | dst);
            } else {
                emit(0x8B000000U | (kScratchA << 16) | (src << 5) | dst);
            }
        }
    }

    // ----- Operands --------------------------------------------------------

    // Eight (or four) bytes of the operand into reg. The scratch registers
    // x15 and x17 are used for addresses.
    void load(const std::uint32_t reg, const Operand& operand,
              const bool wide) {
        const Access slot_access = wide ? Access::load64 : Access::load32;
        switch (operand.kind) {
            case OperandKind::slot:
                memory(slot_access, reg, kSp, slot_offset(operand.value));
                break;
            case OperandKind::immediate:
                move_immediate(reg, static_cast<std::uint64_t>(operand.value),
                               wide);
                break;
            case OperandKind::global:
                address_of(kScratchC, global_at(operand.value));
                memory(slot_access, reg, kScratchC, 0);
                break;
            case OperandKind::indirect:
                memory(Access::load64, kScratchC, kSp,
                       slot_offset(operand.value));
                memory(slot_access, reg, kScratchC, 0);
                break;
            case OperandKind::literal:
                address_of(
                    reg,
                    string_offsets_[static_cast<std::size_t>(operand.value)]);
                break;
            case OperandKind::none:
                break;
        }
    }

    void store(const Operand& operand, const std::uint32_t reg,
               const bool wide) {
        const Access slot_access = wide ? Access::store64 : Access::store32;
        switch (operand.kind) {
            case OperandKind::slot:
                memory(slot_access, reg, kSp, slot_offset(operand.value));
                break;
            case OperandKind::global:
                address_of(kScratchC, global_at(operand.value));
                memory(slot_access, reg, kScratchC, 0);
                break;
            case OperandKind::indirect:
                memory(Access::load64, kScratchC, kSp,
                       slot_offset(operand.value));
                memory(slot_access, reg, kScratchC, 0);
                break;
            case OperandKind::immediate:
            case OperandKind::literal:
            case OperandKind::none:
                break;
        }
    }

    // A double into d<dreg> (through x16).
    void load_double(const std::uint32_t dreg, const Operand& operand) {
        load(kScratchB, operand, true);
        emit(0x9E670000U | (kScratchB << 5) | dreg);  // fmov d, x16
    }

    void store_double(const Operand& operand, const std::uint32_t dreg) {
        emit(0x9E660000U | (dreg << 5) | kScratchB);  // fmov x16, d
        store(operand, kScratchB, true);
    }

    // ----- Jumps -----------------------------------------------------------

    struct FunctionState final {
        std::vector<std::int64_t> label_positions;
        std::vector<Fixup> fixups;
        std::vector<std::size_t> stack_jumps;     // b.lo to the stack stub
        std::vector<std::size_t> overflow_jumps;  // b.cond to the stub
        std::vector<std::size_t> zero_jumps;
    };

    void jump(const int label) {
        state_.fixups.push_back(
            Fixup{image_.size(), label, JumpKind::branch, 0});
        emit(0x14000000U);
    }

    void jump_if(const std::uint32_t condition, const int label) {
        state_.fixups.push_back(
            Fixup{image_.size(), label, JumpKind::conditional, condition});
        emit(0x54000000U | condition);
    }

    void jump_if_zero(const std::uint32_t reg, const bool zero,
                      const int label) {
        state_.fixups.push_back(Fixup{
            image_.size(), label,
            zero ? JumpKind::compare_zero : JumpKind::compare_nonzero, reg});
        emit((zero ? 0x34000000U : 0x35000000U) | reg);
    }

    void overflow_if(const std::uint32_t condition) {
        state_.overflow_jumps.push_back(image_.size());
        emit(0x54000000U | condition);
    }

    void zero_divisor_if(const std::uint32_t condition) {
        state_.zero_jumps.push_back(image_.size());
        emit(0x54000000U | condition);
    }

    static void set_conditional_offset(std::vector<std::uint8_t>& image,
                                       const std::size_t at,
                                       const std::int64_t words) {
        std::uint32_t word = 0;
        for (std::size_t i = 0; i < 4U; ++i) {
            word |= static_cast<std::uint32_t>(image[at + i]) << (8U * i);
        }
        word |= (static_cast<std::uint32_t>(words) & 0x7FFFFU) << 5;
        for (std::size_t i = 0; i < 4U; ++i) {
            image[at + i] = static_cast<std::uint8_t>(word >> (8U * i));
        }
    }

    // A forward branch inside one instruction sequence: the position of the
    // instruction to patch, filled by `resolve_here`.
    std::size_t forward_branch(const std::uint32_t condition) {
        const auto at = image_.size();
        emit(0x54000000U | condition);
        return at;
    }

    std::size_t forward_jump() {
        const auto at = image_.size();
        emit(0x14000000U);
        return at;
    }

    void resolve_conditional(const std::size_t at) {
        set_conditional_offset(image_, at,
                               (static_cast<std::int64_t>(image_.size()) -
                                static_cast<std::int64_t>(at)) /
                                   4);
    }

    void resolve_jump(const std::size_t at) {
        const auto words = (static_cast<std::int64_t>(image_.size()) -
                            static_cast<std::int64_t>(at)) /
                           4;
        patch(at,
              0x14000000U | (static_cast<std::uint32_t>(words) & 0x03FFFFFFU));
    }

    void patch_labels() {
        for (const auto& fixup : state_.fixups) {
            const auto position =
                state_.label_positions[static_cast<std::size_t>(fixup.label)];
            const auto words =
                (position - static_cast<std::int64_t>(fixup.at)) / 4;
            if (fixup.kind == JumpKind::branch) {
                patch(fixup.at,
                      0x14000000U |
                          (static_cast<std::uint32_t>(words) & 0x03FFFFFFU));
            } else {
                set_conditional_offset(image_, fixup.at, words);
            }
        }
    }

    // ----- Checks ----------------------------------------------------------

    // After an arithmetic result in w0: error 6 unless it fits.
    void check_result(const Check check) {
        if (check == Check::int16) {
            emit(0x13003C00U | (0U << 5) | 1U);          // sxth w1, w0
            emit(0x6B00001FU | (1U << 16) | (0U << 5));  // cmp w0, w1
            overflow_if(kNe);
        } else if (check == Check::byte) {
            emit(0x7100001FU | (255U << 10) | (0U << 5));  // cmp w0, #255
            overflow_if(0x8U);                             // b.hi
        }
    }

    // A double result in d0: error 6 when it is infinite or not a number.
    void check_finite() {
        emit(0x9E660000U | (0U << 5) | 0U);  // fmov x0, d0
        move_immediate(1, 0x7FF0000000000000ULL, true);
        emit(0x8A000000U | (1U << 16) | (0U << 5) | 2U);  // and x2, x0, x1
        emit(0xEB00001FU | (1U << 16) | (2U << 5));       // cmp x2, x1
        overflow_if(kEq);
    }

    // ----- Functions -------------------------------------------------------

    void emit_function(const ir::Function& function) {
        state_ = FunctionState{};
        state_.label_positions.assign(
            static_cast<std::size_t>(function.label_count), -1);
        // The outgoing stack arguments.
        std::size_t out_words = 0;
        for (const auto& instruction : function.code) {
            if (instruction.op == Opcode::call ||
                instruction.op == Opcode::runtime) {
                out_words = std::max(out_words, stack_words(instruction));
            }
        }
        out_ = static_cast<std::int64_t>((out_words * 8U + 15U) & ~15U);
        slots_ = function.slot_count;
        fp_offset_ = out_ + 8 * slots_;
        total_ = (fp_offset_ + 16 + 15) & ~15LL;
        // prologue
        if (total_ < 4096) {
            emit(0xD1000000U | (static_cast<std::uint32_t>(total_) << 10) |
                 (31U << 5) | 31U);  // sub sp, sp, #total
        } else {
            move_immediate(kScratchB, static_cast<std::uint64_t>(total_), true);
            emit(0xCB206000U | (kScratchB << 16) | (31U << 5) | 31U);
        }
        // x16 = sp + fp_offset; stp x29, x30, [x16]; mov x29, x16
        add_immediate(kScratchB, kSp, fp_offset_);
        emit(0xA9000000U | (kLr << 10) | (kScratchB << 5) | kFp);
        emit(0x91000000U | (kScratchB << 5) | kFp);
        // Out of stack space: error 28 when sp is below the limit.
        address_of(kScratchB, limit_offset_);
        memory(Access::load64, kScratchB, kScratchB, 0);
        emit(0x91000000U | (kSp << 5) | kScratchC);  // mov x17, sp
        emit(0xEB00001FU | (kScratchB << 16) |
             (kScratchC << 5));  // cmp x17, x16
        state_.stack_jumps.push_back(image_.size());
        emit(0x54000000U | 0x3U);  // b.lo the stack stub
        std::uint32_t int_index = 0;
        std::uint32_t float_index = 0;
        std::int64_t stack_offset = total_ - fp_offset_;
        for (std::size_t i = 0; i < function.parameters.size(); ++i) {
            const auto offset = slot_offset(static_cast<std::int64_t>(i));
            if (function.parameters[i].is_double) {
                if (float_index < 8) {
                    memory(Access::store_double, float_index++, kSp, offset);
                } else {
                    memory(Access::load64, kScratchB, kFp, stack_offset);
                    memory(Access::store64, kScratchB, kSp, offset);
                    stack_offset += 8;
                }
            } else if (int_index < 8) {
                memory(Access::store64, int_index++, kSp, offset);
            } else {
                memory(Access::load64, kScratchB, kFp, stack_offset);
                memory(Access::store64, kScratchB, kSp, offset);
                stack_offset += 8;
            }
        }
        for (const auto& instruction : function.code) {
            emit_instruction(function, instruction);
        }
        emit_stubs(function);
        patch_labels();
    }

    [[nodiscard]] static std::size_t stack_words(
        const ir::Instruction& instruction) {
        std::size_t ints = 0;
        std::size_t floats = 0;
        std::size_t words = 0;
        for (std::size_t i = 0; i < instruction.args.size(); ++i) {
            const bool is_double = ((instruction.double_args >> i) & 1U) != 0U;
            std::size_t& counter = is_double ? floats : ints;
            if (counter < 8) {
                ++counter;
            } else {
                ++words;
            }
        }
        return words;
    }

    void emit_epilogue() {
        // mov x16, x29; ldp x29, x30, [x16]; add sp, x16, #rest; ret
        emit(0x91000000U | (kFp << 5) | kScratchB);
        emit(0xA9400000U | (kLr << 10) | (kScratchB << 5) | kFp);
        emit(0x91000000U |
             (static_cast<std::uint32_t>(total_ - fp_offset_) << 10) |
             (kScratchB << 5) | kSp);
        emit(0xD65F03C0U);
    }

    void emit_stubs(const ir::Function& function) {
        if (!state_.stack_jumps.empty()) {
            for (const auto at : state_.stack_jumps) {
                resolve_conditional(at);
            }
            move_immediate(0, 28, false);
            call_service(ir::Service::raise_error);
            emit_epilogue();  // the caller sees the pending error
        }
        const auto stub = [&](std::vector<std::size_t>& jumps,
                              const int number) {
            if (jumps.empty()) {
                return;
            }
            for (const auto at : jumps) {
                resolve_conditional(at);
            }
            move_immediate(0, static_cast<std::uint64_t>(number), false);
            call_service(ir::Service::raise_error);
            jump(function.error_label);
        };
        stub(state_.overflow_jumps, 6);
        stub(state_.zero_jumps, 11);
    }

    void call_service(const ir::Service service) {
        address_of(kScratchB, 0);  // the entry table
        memory(Access::load64, kScratchB, kScratchB,
               static_cast<std::int64_t>(service) * 8);
        emit(0xD63F0000U | (kScratchB << 5));  // blr x16
    }

    // Loads the arguments into their registers and stack words.
    void load_arguments(const ir::Instruction& instruction) {
        std::uint32_t int_index = 0;
        std::uint32_t float_index = 0;
        std::int64_t stack_index = 0;
        for (std::size_t i = 0; i < instruction.args.size(); ++i) {
            const Operand& operand = instruction.args[i];
            const bool is_double = ((instruction.double_args >> i) & 1U) != 0U;
            if (is_double) {
                if (float_index < 8) {
                    load_double(float_index++, operand);
                } else {
                    load(kScratchB, operand, true);
                    memory(Access::store64, kScratchB, 31, 8 * stack_index++);
                }
            } else if (int_index < 8) {
                load(int_index++, operand, true);
            } else {
                load(kScratchB, operand, true);
                memory(Access::store64, kScratchB, 31, 8 * stack_index++);
            }
        }
    }

    void store_result(const ir::Instruction& instruction) {
        if (instruction.dst.kind == OperandKind::none) {
            return;
        }
        if (instruction.double_result) {
            store_double(instruction.dst, 0);
        } else {
            store(instruction.dst, 0, instruction.wide);
        }
    }

    void emit_instruction(const ir::Function& function,
                          const ir::Instruction& instruction) {
        switch (instruction.op) {
            case Opcode::move:
                load(9, instruction.a, instruction.wide);
                store(instruction.dst, 9, instruction.wide);
                break;
            case Opcode::add:
            case Opcode::sub: {
                load(0, instruction.a, false);
                load(1, instruction.b, false);
                const bool add = instruction.op == Opcode::add;
                if (instruction.check == Check::int32) {
                    emit((add ? 0x2B000000U : 0x6B000000U) | (1U << 16) |
                         (0U << 5) | 0U);  // adds/subs w0, w0, w1
                    overflow_if(kVs);
                } else {
                    emit((add ? 0x0B000000U : 0x4B000000U) | (1U << 16) |
                         (0U << 5) | 0U);
                    check_result(instruction.check);
                }
                store(instruction.dst, 0, false);
                break;
            }
            case Opcode::mul:
                load(0, instruction.a, false);
                load(1, instruction.b, false);
                if (instruction.check == Check::int32) {
                    emit(0x9B207C00U | (1U << 16) | (0U << 5) |
                         2U);  // smull x2
                    emit(0xEB20C01FU | (2U << 16) |
                         (2U << 5));  // cmp x2, w2 sxtw
                    overflow_if(kNe);
                    emit(0x2A0003E0U | (2U << 16) | 0U);  // mov w0, w2
                } else {
                    emit(0x1B007C00U | (1U << 16) | (0U << 5) | 0U);  // mul
                    check_result(instruction.check);
                }
                store(instruction.dst, 0, false);
                break;
            case Opcode::bit_and:
            case Opcode::bit_or:
            case Opcode::bit_xor: {
                load(0, instruction.a, false);
                load(1, instruction.b, false);
                const std::uint32_t base =
                    instruction.op == Opcode::bit_and
                        ? 0x0A000000U
                        : (instruction.op == Opcode::bit_or ? 0x2A000000U
                                                            : 0x4A000000U);
                emit(base | (1U << 16) | (0U << 5) | 0U);
                store(instruction.dst, 0, false);
                break;
            }
            case Opcode::divide:
            case Opcode::modulo:
                emit_division(instruction);
                break;
            case Opcode::negate:
                load(0, instruction.a, false);
                if (instruction.check == Check::int32) {
                    emit(0x6B0003E0U | (0U << 16) | 0U);  // negs w0, w0
                    overflow_if(kVs);
                } else {
                    emit(0x4B0003E0U | (0U << 16) | 0U);  // neg w0, w0
                    check_result(instruction.check);
                }
                store(instruction.dst, 0, false);
                break;
            case Opcode::bit_not:
                load(0, instruction.a, false);
                emit(0x2A2003E0U | (0U << 16) | 0U);  // mvn w0, w0
                store(instruction.dst, 0, false);
                break;
            case Opcode::compare: {
                load(0, instruction.a, false);
                load(1, instruction.b, false);
                emit(0x6B00001FU | (1U << 16) | (0U << 5));  // cmp w0, w1
                emit_cset_negated(integer_condition(instruction.condition));
                store(instruction.dst, 0, false);
                break;
            }
            case Opcode::fadd:
            case Opcode::fsub:
            case Opcode::fmul:
            case Opcode::fdivide: {
                load_double(0, instruction.a);
                load_double(1, instruction.b);
                if (instruction.op == Opcode::fdivide) {
                    emit(0x1E602008U | (1U << 5));  // fcmp d1, #0.0
                    zero_divisor_if(kEq);
                }
                const std::uint32_t base =
                    instruction.op == Opcode::fadd
                        ? 0x1E612800U
                        : (instruction.op == Opcode::fsub
                               ? 0x1E613800U
                               : (instruction.op == Opcode::fmul
                                      ? 0x1E610800U
                                      : 0x1E611800U));
                emit(base | (1U << 16) | (0U << 5) | 0U);
                check_finite();
                store_double(instruction.dst, 0);
                break;
            }
            case Opcode::fnegate:
                load_double(0, instruction.a);
                emit(0x1E614000U | (0U << 5) | 0U);  // fneg d0, d0
                store_double(instruction.dst, 0);
                break;
            case Opcode::fcompare:
                load_double(0, instruction.a);
                load_double(1, instruction.b);
                emit(0x1E602000U | (1U << 16) | (0U << 5));  // fcmp d0, d1
                emit_cset_negated(float_condition(instruction.condition));
                store(instruction.dst, 0, false);
                break;
            case Opcode::int_to_double:
                load(0, instruction.a, false);
                emit(0x1E620000U | (0U << 5) | 0U);  // scvtf d0, w0
                store_double(instruction.dst, 0);
                break;
            case Opcode::round_single:
                load_double(0, instruction.a);
                emit(0x1E624000U | (0U << 5) | 0U);  // fcvt s0, d0
                emit(0x1E22C000U | (0U << 5) | 0U);  // fcvt d0, s0
                check_finite();
                store_double(instruction.dst, 0);
                break;
            case Opcode::narrow:
                load(0, instruction.a, false);
                check_result(instruction.check == Check::byte ? Check::byte
                                                              : Check::int16);
                store(instruction.dst, 0, false);
                break;
            case Opcode::address:
                if (instruction.a.kind == OperandKind::global) {
                    address_of(0, global_at(instruction.a.value));
                } else {
                    add_immediate(0, kSp, slot_offset(instruction.a.value));
                }
                store(instruction.dst, 0, true);
                break;
            case Opcode::pointer_add:
                load(0, instruction.a, true);
                if (instruction.b.value >= 0) {
                    add_immediate(0, 0, instruction.b.value);
                } else {
                    move_immediate(
                        1, static_cast<std::uint64_t>(instruction.b.value),
                        true);
                    emit(0x8B000000U | (1U << 16) | (0U << 5) | 0U);
                }
                store(instruction.dst, 0, true);
                break;
            case Opcode::check_error:
                address_of(kScratchB, flag_offset_);
                memory(Access::load32, kScratchB, kScratchB, 0);
                jump_if_zero(kScratchB, false, instruction.target);
                break;
            case Opcode::jump:
                jump(instruction.target);
                break;
            case Opcode::branch_zero:
            case Opcode::branch_nonzero:
                load(0, instruction.a, false);
                jump_if_zero(0, instruction.op == Opcode::branch_zero,
                             instruction.target);
                break;
            case Opcode::label:
                state_.label_positions[static_cast<std::size_t>(
                    instruction.target)] =
                    static_cast<std::int64_t>(image_.size());
                break;
            case Opcode::call:
                load_arguments(instruction);
                call_fixups_.emplace_back(image_.size(), instruction.target);
                emit(0x94000000U);
                store_result(instruction);
                break;
            case Opcode::runtime:
                load_arguments(instruction);
                call_service(static_cast<ir::Service>(instruction.target));
                store_result(instruction);
                break;
            case Opcode::print_text: {
                const auto index = static_cast<std::size_t>(instruction.target);
                address_of(0, string_offsets_[index] + 8U);
                move_immediate(
                    1,
                    static_cast<std::uint64_t>(program_.strings[index].size()),
                    false);
                call_service(ir::Service::print_text);
                break;
            }
            case Opcode::ret:
                if (function.result_slot >= 0) {
                    if (function.result_double) {
                        load_double(0, ir::slot(function.result_slot));
                    } else {
                        load(0, ir::slot(function.result_slot), true);
                    }
                }
                // mov x16, x29; ldp x29, x30, [x16]; add sp, x16, #rest; ret
                emit(0x91000000U | (kFp << 5) | kScratchB);
                emit(0xA9400000U | (kLr << 10) | (kScratchB << 5) | kFp);
                emit(0x91000000U |
                     (static_cast<std::uint32_t>(total_ - fp_offset_) << 10) |
                     (kScratchB << 5) | kSp);
                emit(0xD65F03C0U);
                break;
        }
    }

    // After a compare: w0 = -1 when the condition holds, else 0.
    void emit_cset_negated(const std::uint32_t condition) {
        // cset w0, cond is csinc w0, wzr, wzr, !cond
        emit(0x1A9F07E0U | ((condition ^ 1U) << 12) | 0U);
        emit(0x4B0003E0U | (0U << 16) | 0U);  // neg w0, w0
    }

    void emit_division(const ir::Instruction& instruction) {
        const bool is_division = instruction.op == Opcode::divide;
        load(0, instruction.a, false);
        load(1, instruction.b, false);
        jump_if_zero_local(1);
        emit(0x3100041FU | (1U << 5));  // cmn w1, #1
        const auto normal = forward_branch(kNe);
        if (is_division) {
            if (instruction.check == Check::int32) {
                emit(0x6B0003E0U | (0U << 16) | 0U);  // negs w0, w0: a \ -1
                overflow_if(kVs);
            } else {
                emit(0x4B0003E0U | (0U << 16) | 0U);  // neg w0, w0
                check_result(instruction.check);
            }
        } else {
            move_immediate(0, 0, false);  // a Mod -1
        }
        const auto done = forward_jump();
        resolve_conditional(normal);
        emit(0x1AC00C00U | (1U << 16) | (0U << 5) | 2U);  // sdiv w2, w0, w1
        if (is_division) {
            emit(0x2A0003E0U | (2U << 16) | 0U);  // mov w0, w2
        } else {
            // msub w0, w2, w1, w0
            emit(0x1B008000U | (1U << 16) | (0U << 10) | (2U << 5) | 0U);
        }
        resolve_jump(done);
        store(instruction.dst, 0, false);
    }

    // cbz wN to the zero-divisor stub: a compare then b.eq
    void jump_if_zero_local(const std::uint32_t reg) {
        emit(0x7100001FU | (reg << 5));  // cmp wN, #0
        zero_divisor_if(kEq);
    }

    const ir::Program& program_;
    std::vector<std::uint8_t> image_;
    std::size_t flag_offset_{};
    std::size_t limit_offset_{};
    std::size_t globals_offset_{};
    std::vector<std::size_t> string_offsets_;
    std::vector<std::pair<std::size_t, int>> call_fixups_;
    FunctionState state_;
    std::int64_t out_{};
    std::int64_t total_{};
    std::int64_t slots_{};
    std::int64_t fp_offset_{};
};

}  // namespace

BackEndResult generate_arm64(const ir::Program& program) {
    Emitter emitter(program);
    return emitter.run();
}

}  // namespace wfc::compiler
