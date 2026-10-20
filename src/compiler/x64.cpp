// The AMD64 back end: IR to machine code for the Windows x64 calling
// convention. It keeps every value in its frame slot and uses eax, ecx and edx
// as scratch registers, so the emitted code is slow but small and obviously
// correct; register allocation comes after the corpus passes (ADR-0008).

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>

#include "native_image.hpp"

namespace wfc::compiler {

namespace {

using ir::Check;
using ir::Condition;
using ir::Opcode;
using ir::Operand;
using ir::OperandKind;

constexpr int kEax = 0;
constexpr int kEcx = 1;
constexpr int kEdx = 2;
constexpr int kR8 = 8;
constexpr int kR9 = 9;
constexpr int kR11 = 11;

constexpr std::uint8_t kCcOverflow = 0x0;
constexpr std::uint8_t kCcEqual = 0x4;
constexpr std::uint8_t kCcNotEqual = 0x5;
constexpr std::uint8_t kCcLess = 0xC;
constexpr std::uint8_t kCcGreaterEqual = 0xD;
constexpr std::uint8_t kCcLessEqual = 0xE;
constexpr std::uint8_t kCcGreater = 0xF;

[[nodiscard]] std::uint8_t condition_code(const Condition condition) {
    switch (condition) {
        case Condition::equal:
            return kCcEqual;
        case Condition::not_equal:
            return kCcNotEqual;
        case Condition::less:
            return kCcLess;
        case Condition::less_equal:
            return kCcLessEqual;
        case Condition::greater:
            return kCcGreater;
        case Condition::greater_equal:
            return kCcGreaterEqual;
    }
    return kCcEqual;
}

struct Fixup final {
    std::size_t at{};  // the position of a 32-bit displacement
    int label{};
};

class Emitter final {
public:
    explicit Emitter(const ir::Program& program) : program_(program) {}

    BackEndResult run() {
        layout();
        std::vector<std::size_t> function_starts;
        std::vector<std::pair<std::size_t, int>> call_fixups;
        for (const auto& function : program_.functions) {
            function_starts.push_back(image_.size());
            emit_function(function, call_fixups);
        }
        for (const auto& [at, callee] : call_fixups) {
            patch32(at, static_cast<std::int32_t>(
                            function_starts[static_cast<std::size_t>(callee)] -
                            (at + 4U)));
        }
        BackEndResult result;
        result.image.architecture = Architecture::x64;
        result.image.table_count = static_cast<std::size_t>(ir::Service::count);
        result.image.entry =
            function_starts[static_cast<std::size_t>(program_.entry)];
        result.image.bytes = std::move(image_);
        return result;
    }

private:
    // ----- Layout ----------------------------------------------------------

    void layout() {
        table_offset_ = 0;
        std::size_t at = static_cast<std::size_t>(ir::Service::count) * 8U;
        globals_offset_ = at;
        at += program_.globals.size() * 8U;
        for (const auto& text : program_.strings) {
            string_offsets_.push_back(at);
            at += text.size();
        }
        at = (at + 15U) & ~static_cast<std::size_t>(15U);
        image_.assign(at, 0);
        for (std::size_t i = 0; i < program_.strings.size(); ++i) {
            std::memcpy(image_.data() + string_offsets_[i],
                        program_.strings[i].data(), program_.strings[i].size());
        }
    }

    // ----- Byte helpers ----------------------------------------------------

    void byte(const std::uint8_t value) { image_.push_back(value); }
    void bytes(std::initializer_list<std::uint8_t> values) {
        for (const auto value : values) {
            image_.push_back(value);
        }
    }
    void dword(const std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8) {
            image_.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    }
    void patch32(const std::size_t at, const std::int32_t value) {
        const auto raw = static_cast<std::uint32_t>(value);
        for (std::size_t i = 0; i < 4U; ++i) {
            image_[at + i] = static_cast<std::uint8_t>(raw >> (8U * i));
        }
    }

    // REX prefix for `reg` and `rm` (when needed or when `wide`).
    void rex(const bool wide, const int reg, const int rm) {
        const std::uint8_t value = static_cast<std::uint8_t>(
            0x40 | (wide ? 8 : 0) | (((reg >> 3) & 1) << 2) | ((rm >> 3) & 1));
        if (value != 0x40) {
            byte(value);
        }
    }

    [[nodiscard]] static std::int32_t slot_disp(const std::int64_t slot) {
        return static_cast<std::int32_t>(-8 * (slot + 1));
    }

    // opcode with a [rbp + disp] operand
    void op_rbp(const bool wide, const std::uint8_t opcode, const int reg,
                const std::int32_t disp) {
        rex(wide, reg, 0);
        byte(opcode);
        byte(static_cast<std::uint8_t>(0x80 | ((reg & 7) << 3) | 5));
        dword(static_cast<std::uint32_t>(disp));
    }

    // opcode with a [rip + offset-of-image-position] operand
    void op_rip(const bool wide, const std::vector<std::uint8_t> opcode,
                const int reg, const std::size_t target) {
        rex(wide, reg, 0);
        for (const auto value : opcode) {
            byte(value);
        }
        byte(static_cast<std::uint8_t>(((reg & 7) << 3) | 5));
        const std::size_t after = image_.size() + 4U;
        dword(static_cast<std::uint32_t>(static_cast<std::int64_t>(target) -
                                         static_cast<std::int64_t>(after)));
    }

    // opcode with a [r11] operand
    void op_r11(const std::uint8_t opcode, const int reg) {
        rex(false, reg, kR11);
        byte(opcode);
        byte(static_cast<std::uint8_t>(((reg & 7) << 3) | (kR11 & 7)));
    }

    // ----- Operands --------------------------------------------------------

    void load32(const int reg, const Operand& operand) {
        switch (operand.kind) {
            case OperandKind::slot:
                op_rbp(false, 0x8B, reg, slot_disp(operand.value));
                break;
            case OperandKind::immediate:
                rex(false, 0, reg);
                byte(static_cast<std::uint8_t>(0xB8 + (reg & 7)));
                dword(static_cast<std::uint32_t>(
                    static_cast<std::int32_t>(operand.value)));
                break;
            case OperandKind::global:
                op_rip(false, {0x8B}, reg, global_offset(operand.value));
                break;
            case OperandKind::indirect:
                op_rbp(true, 0x8B, kR11, slot_disp(operand.value));
                op_r11(0x8B, reg);
                break;
            case OperandKind::none:
                break;
        }
    }

    // The 64-bit value of a slot (a pointer) or an ordinary value.
    void load_argument(const int reg, const Operand& operand) {
        if (operand.kind == OperandKind::slot) {
            op_rbp(true, 0x8B, reg, slot_disp(operand.value));
        } else {
            load32(reg, operand);
        }
    }

    void store32(const Operand& operand, const int reg) {
        switch (operand.kind) {
            case OperandKind::slot:
                op_rbp(false, 0x89, reg, slot_disp(operand.value));
                break;
            case OperandKind::global:
                op_rip(false, {0x89}, reg, global_offset(operand.value));
                break;
            case OperandKind::indirect:
                op_rbp(true, 0x8B, kR11, slot_disp(operand.value));
                op_r11(0x89, reg);
                break;
            case OperandKind::immediate:
            case OperandKind::none:
                break;
        }
    }

    [[nodiscard]] std::size_t global_offset(const std::int64_t index) const {
        return globals_offset_ + static_cast<std::size_t>(index) * 8U;
    }

    // ----- Labels and jumps ------------------------------------------------

    struct FunctionState final {
        std::vector<std::int64_t> label_positions;
        std::vector<Fixup> fixups;
        std::vector<Fixup> overflow_jumps;
        std::vector<Fixup> zero_jumps;
        int outgoing{};
    };

    void jump_to(const std::uint8_t first, const std::uint8_t second,
                 const int label) {
        if (first != 0) {
            byte(first);
        }
        byte(second);
        state_.fixups.push_back(Fixup{image_.size(), label});
        dword(0);
    }

    void jcc(const std::uint8_t cc, const int label) {
        jump_to(0x0F, static_cast<std::uint8_t>(0x80 | cc), label);
    }

    void jmp(const int label) { jump_to(0, 0xE9, label); }

    void jcc_overflow(const std::uint8_t cc) {
        bytes({0x0F, static_cast<std::uint8_t>(0x80 | cc)});
        state_.overflow_jumps.push_back(Fixup{image_.size(), 0});
        dword(0);
    }

    void jcc_zero_divisor() {
        bytes({0x0F, 0x84});
        state_.zero_jumps.push_back(Fixup{image_.size(), 0});
        dword(0);
    }

    // After an arithmetic result in eax: error 6 unless it fits.
    void check_result(const Check check) {
        if (check == Check::int32) {
            jcc_overflow(kCcOverflow);
        } else if (check == Check::int16) {
            bytes({0x0F, 0xBF, 0xD0});  // movsx edx, ax
            bytes({0x3B, 0xD0});        // cmp edx, eax
            jcc_overflow(kCcNotEqual);
        }
    }

    // ----- Functions -------------------------------------------------------

    void emit_function(const ir::Function& function,
                       std::vector<std::pair<std::size_t, int>>& call_fixups) {
        state_ = FunctionState{};
        state_.label_positions.assign(
            static_cast<std::size_t>(function.label_count), -1);
        int max_arguments = 0;
        for (const auto& instruction : function.code) {
            max_arguments = std::max(max_arguments,
                                     static_cast<int>(instruction.args.size()));
        }
        state_.outgoing = 32 + 8 * std::max(0, max_arguments - 4);
        const std::int32_t frame = static_cast<std::int32_t>(
            (8 * function.slot_count + state_.outgoing + 15) & ~15);
        // prologue
        byte(0x55);                 // push rbp
        bytes({0x48, 0x89, 0xE5});  // mov rbp, rsp
        bytes({0x48, 0x81, 0xEC});  // sub rsp, frame
        dword(static_cast<std::uint32_t>(frame));
        for (std::size_t i = 0; i < function.parameters.size(); ++i) {
            const bool wide = !function.parameters[i].by_value;
            const std::int32_t disp = slot_disp(static_cast<std::int64_t>(i));
            if (i < 4U) {
                const int reg = i == 0   ? kEcx
                                : i == 1 ? kEdx
                                : i == 2 ? kR8
                                         : kR9;
                op_rbp(wide, 0x89, reg, disp);
            } else {
                const auto from = static_cast<std::int32_t>(
                    48 + 8 * static_cast<std::int32_t>(i - 4U));
                op_rbp(true, 0x8B, kEax, from);
                op_rbp(true, 0x89, kEax, disp);
            }
        }
        for (const auto& instruction : function.code) {
            emit_instruction(function, instruction, call_fixups);
        }
        // Every function ends with `ret`; the stubs follow it.
        patch_labels();
        emit_stubs();
    }

    void emit_stubs() {
        if (!state_.overflow_jumps.empty()) {
            const std::size_t here = image_.size();
            for (const auto& jump : state_.overflow_jumps) {
                patch32(jump.at,
                        static_cast<std::int32_t>(here - (jump.at + 4U)));
            }
            emit_error_call(6);
        }
        if (!state_.zero_jumps.empty()) {
            const std::size_t here = image_.size();
            for (const auto& jump : state_.zero_jumps) {
                patch32(jump.at,
                        static_cast<std::int32_t>(here - (jump.at + 4U)));
            }
            emit_error_call(11);
        }
    }

    void emit_error_call(const int number) {
        load32(kEcx, ir::immediate(number));
        call_service(ir::Service::raise_error);
        byte(0xCC);  // int3: the service does not return
    }

    void call_service(const ir::Service service) {
        bytes({0xFF, 0x15});  // call [rip + disp32]
        const std::size_t target =
            table_offset_ + static_cast<std::size_t>(service) * 8U;
        const std::size_t after = image_.size() + 4U;
        dword(static_cast<std::uint32_t>(static_cast<std::int64_t>(target) -
                                         static_cast<std::int64_t>(after)));
    }

    void patch_labels() {
        for (const auto& fixup : state_.fixups) {
            const auto position =
                state_.label_positions[static_cast<std::size_t>(fixup.label)];
            patch32(fixup.at,
                    static_cast<std::int32_t>(
                        position - static_cast<std::int64_t>(fixup.at + 4U)));
        }
    }

    void emit_instruction(
        const ir::Function& function, const ir::Instruction& instruction,
        std::vector<std::pair<std::size_t, int>>& call_fixups) {
        switch (instruction.op) {
            case Opcode::move:
                load32(kEax, instruction.a);
                store32(instruction.dst, kEax);
                break;
            case Opcode::add:
            case Opcode::sub:
            case Opcode::mul:
            case Opcode::bit_and:
            case Opcode::bit_or:
            case Opcode::bit_xor:
                load32(kEax, instruction.a);
                load32(kEcx, instruction.b);
                switch (instruction.op) {
                    case Opcode::add:
                        bytes({0x03, 0xC1});
                        break;
                    case Opcode::sub:
                        bytes({0x2B, 0xC1});
                        break;
                    case Opcode::mul:
                        bytes({0x0F, 0xAF, 0xC1});
                        break;
                    case Opcode::bit_and:
                        bytes({0x23, 0xC1});
                        break;
                    case Opcode::bit_or:
                        bytes({0x0B, 0xC1});
                        break;
                    default:
                        bytes({0x33, 0xC1});
                        break;
                }
                check_result(instruction.check);
                store32(instruction.dst, kEax);
                break;
            case Opcode::divide:
            case Opcode::modulo:
                emit_division(instruction);
                break;
            case Opcode::negate:
                load32(kEax, instruction.a);
                bytes({0xF7, 0xD8});  // neg eax
                check_result(instruction.check);
                store32(instruction.dst, kEax);
                break;
            case Opcode::bit_not:
                load32(kEax, instruction.a);
                bytes({0xF7, 0xD0});  // not eax
                store32(instruction.dst, kEax);
                break;
            case Opcode::compare:
                load32(kEax, instruction.a);
                load32(kEcx, instruction.b);
                bytes({0x3B, 0xC1});  // cmp eax, ecx
                bytes({0x0F,
                       static_cast<std::uint8_t>(
                           0x90 | condition_code(instruction.condition)),
                       0xC0});              // setcc al
                bytes({0x0F, 0xB6, 0xC0});  // movzx eax, al
                bytes({0xF7, 0xD8});        // neg eax: 1 -> -1
                store32(instruction.dst, kEax);
                break;
            case Opcode::narrow:
                load32(kEax, instruction.a);
                check_result(Check::int16);
                store32(instruction.dst, kEax);
                break;
            case Opcode::address:
                if (instruction.a.kind == OperandKind::global) {
                    op_rip(true, {0x8D}, kEax,
                           global_offset(instruction.a.value));
                } else {
                    op_rbp(true, 0x8D, kEax, slot_disp(instruction.a.value));
                }
                op_rbp(true, 0x89, kEax, slot_disp(instruction.dst.value));
                break;
            case Opcode::jump:
                jmp(instruction.target);
                break;
            case Opcode::branch_zero:
            case Opcode::branch_nonzero:
                load32(kEax, instruction.a);
                bytes({0x85, 0xC0});  // test eax, eax
                jcc(instruction.op == Opcode::branch_zero ? kCcEqual
                                                          : kCcNotEqual,
                    instruction.target);
                break;
            case Opcode::label:
                state_.label_positions[static_cast<std::size_t>(
                    instruction.target)] =
                    static_cast<std::int64_t>(image_.size());
                break;
            case Opcode::call:
                emit_call(instruction, call_fixups);
                break;
            case Opcode::runtime:
                emit_arguments(instruction.args);
                call_service(static_cast<ir::Service>(instruction.target));
                break;
            case Opcode::print_text: {
                const auto index = static_cast<std::size_t>(instruction.target);
                op_rip(true, {0x8D}, kEcx, string_offsets_[index]);  // lea rcx
                load32(kEdx, ir::immediate(static_cast<std::int64_t>(
                                 program_.strings[index].size())));
                call_service(ir::Service::print_text);
                break;
            }
            case Opcode::ret:
                if (function.result_slot >= 0) {
                    load32(kEax, ir::slot(function.result_slot));
                }
                byte(0xC9);  // leave
                byte(0xC3);  // ret
                break;
        }
    }

    void emit_division(const ir::Instruction& instruction) {
        const bool is_division = instruction.op == Opcode::divide;
        load32(kEax, instruction.a);
        load32(kEcx, instruction.b);
        bytes({0x85, 0xC9});  // test ecx, ecx
        jcc_zero_divisor();
        bytes({0x83, 0xF9, 0xFF});  // cmp ecx, -1
        bytes({0x0F, 0x85});        // jne normal
        const std::size_t normal_jump = image_.size();
        dword(0);
        if (is_division) {
            bytes({0xF7, 0xD8});  // neg eax: a \ -1
            check_result(instruction.check);
        } else {
            bytes({0x33, 0xC0});  // xor eax, eax: a Mod -1
        }
        byte(0xE9);  // jmp done
        const std::size_t done_jump = image_.size();
        dword(0);
        patch32(normal_jump,
                static_cast<std::int32_t>(image_.size() - (normal_jump + 4U)));
        byte(0x99);           // cdq
        bytes({0xF7, 0xF9});  // idiv ecx
        if (!is_division) {
            bytes({0x8B, 0xC2});  // mov eax, edx
        }
        patch32(done_jump,
                static_cast<std::int32_t>(image_.size() - (done_jump + 4U)));
        store32(instruction.dst, kEax);
    }

    // Loads the arguments of a call into rcx, rdx, r8, r9 and the stack.
    void emit_arguments(const std::vector<Operand>& args) {
        static constexpr int registers[4] = {kEcx, kEdx, kR8, kR9};
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i < 4U) {
                load_argument(registers[i], args[i]);
            } else {
                load_argument(kEax, args[i]);
                // mov [rsp + 32 + 8 * (i - 4)], rax
                bytes({0x48, 0x89, 0x84, 0x24});
                dword(static_cast<std::uint32_t>(
                    32 + 8 * static_cast<std::int32_t>(i - 4U)));
            }
        }
    }

    void emit_call(const ir::Instruction& instruction,
                   std::vector<std::pair<std::size_t, int>>& call_fixups) {
        emit_arguments(instruction.args);
        byte(0xE8);  // call rel32
        call_fixups.emplace_back(image_.size(), instruction.target);
        dword(0);
        store32(instruction.dst, kEax);
    }

    const ir::Program& program_;
    std::vector<std::uint8_t> image_;
    std::size_t table_offset_{};
    std::size_t globals_offset_{};
    std::vector<std::size_t> string_offsets_;
    FunctionState state_;
};

}  // namespace

BackEndResult generate_x64(const ir::Program& program) {
    Emitter emitter(program);
    return emitter.run();
}

}  // namespace wfc::compiler
