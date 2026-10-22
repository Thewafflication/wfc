// The x86 back end: IR to 32-bit machine code with the cdecl calling
// convention (arguments on the stack, the caller cleans up, a double comes
// back in ST0). Like the AMD64 back end it keeps every value in its frame slot
// and uses eax, ecx and edx as scratch registers.
//
// The image is position independent without relocations: every function
// computes the address of the image in ebx (`call $+5 / pop ebx`) and reaches
// the entry table, the pending-error flag, the globals and the string literals
// through it. ebx is saved in the frame because cdecl keeps it for the caller.
//
// Frame (higher addresses first): return address, saved ebp, then
//   [ebp-4]   the caller's ebx
//   [ebp-8]   unused
//   slot k at [ebp-16-8k]
// A slot is eight bytes; an eight-byte value is a double or a pointer-sized
// handle whose high half is zero or unused.

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

constexpr int kEax = 0;
constexpr int kEcx = 1;
constexpr int kEdx = 2;

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
        result.image.architecture = Architecture::x86;
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
        std::size_t at = static_cast<std::size_t>(ir::Service::count) * 4U;
        at = (at + 7U) & ~static_cast<std::size_t>(7U);
        flag_offset_ = at;  // the pending-error flag, then the globals
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

    [[nodiscard]] static std::int32_t slot_disp(const std::int64_t slot) {
        return static_cast<std::int32_t>(-16 - 8 * slot);
    }

    [[nodiscard]] std::int32_t global_disp(const std::int64_t index) const {
        return static_cast<std::int32_t>(globals_offset_ +
                                         static_cast<std::size_t>(index) * 8U);
    }

    // ----- Addressing: [ebp+d], [ebx+d] and [reg] --------------------------

    // opcode (and a ModRM with a 32-bit displacement) for [ebp + disp]
    void op_ebp(const std::uint8_t opcode, const int reg,
                const std::int32_t disp) {
        byte(opcode);
        byte(static_cast<std::uint8_t>(0x80 | (reg << 3) | 5));
        dword(static_cast<std::uint32_t>(disp));
    }

    // ... for [ebx + disp]
    void op_ebx(const std::uint8_t opcode, const int reg,
                const std::int32_t disp) {
        byte(opcode);
        byte(static_cast<std::uint8_t>(0x80 | (reg << 3) | 3));
        dword(static_cast<std::uint32_t>(disp));
    }

    // ... for [base + disp8] where base is eax, ecx or edx
    void op_reg(const std::uint8_t opcode, const int reg, const int base,
                const std::uint8_t disp = 0) {
        byte(opcode);
        if (disp == 0) {
            byte(static_cast<std::uint8_t>((reg << 3) | base));
        } else {
            byte(static_cast<std::uint8_t>(0x40 | (reg << 3) | base));
            byte(disp);
        }
    }

    // ----- Operands --------------------------------------------------------

    // Four bytes into `reg`.
    void load32(const int reg, const Operand& operand) {
        switch (operand.kind) {
            case OperandKind::slot:
                op_ebp(0x8B, reg, slot_disp(operand.value));
                break;
            case OperandKind::immediate:
                byte(static_cast<std::uint8_t>(0xB8 + reg));
                dword(static_cast<std::uint32_t>(
                    static_cast<std::int32_t>(operand.value)));
                break;
            case OperandKind::global:
                op_ebx(0x8B, reg, global_disp(operand.value));
                break;
            case OperandKind::indirect:
                op_ebp(0x8B, reg, slot_disp(operand.value));
                op_reg(0x8B, reg, reg);
                break;
            case OperandKind::literal:
                op_ebx(0x8D, reg,
                       static_cast<std::int32_t>(
                           string_offsets_[static_cast<std::size_t>(
                               operand.value)]));
                break;
            case OperandKind::none:
                break;
        }
    }

    // Four bytes from `reg`.
    void store32(const Operand& operand, const int reg) {
        switch (operand.kind) {
            case OperandKind::slot:
                op_ebp(0x89, reg, slot_disp(operand.value));
                break;
            case OperandKind::global:
                op_ebx(0x89, reg, global_disp(operand.value));
                break;
            case OperandKind::indirect: {
                const int pointer = reg == kEdx ? kEcx : kEdx;
                op_ebp(0x8B, pointer, slot_disp(operand.value));
                op_reg(0x89, reg, pointer);
                break;
            }
            case OperandKind::immediate:
            case OperandKind::literal:
            case OperandKind::none:
                break;
        }
    }

    // Eight bytes into eax (low) and edx (high): a double, or a handle or
    // pointer whose high half is zero. An immediate holds the whole bits.
    void load_pair(const Operand& operand) {
        switch (operand.kind) {
            case OperandKind::slot:
                op_ebp(0x8B, kEax, slot_disp(operand.value));
                op_ebp(0x8B, kEdx, slot_disp(operand.value) + 4);
                break;
            case OperandKind::global:
                op_ebx(0x8B, kEax, global_disp(operand.value));
                op_ebx(0x8B, kEdx, global_disp(operand.value) + 4);
                break;
            case OperandKind::indirect:
                op_ebp(0x8B, kEcx, slot_disp(operand.value));
                op_reg(0x8B, kEax, kEcx);
                op_reg(0x8B, kEdx, kEcx, 4);
                break;
            case OperandKind::literal:
                load32(kEax, operand);
                bytes({0x33, 0xD2});  // xor edx, edx
                break;
            case OperandKind::immediate: {
                const auto bits = static_cast<std::uint64_t>(operand.value);
                byte(static_cast<std::uint8_t>(0xB8 + kEax));
                dword(static_cast<std::uint32_t>(bits));
                byte(static_cast<std::uint8_t>(0xB8 + kEdx));
                dword(static_cast<std::uint32_t>(bits >> 32U));
                break;
            }
            case OperandKind::none:
                break;
        }
    }

    // Eight bytes from eax (low) and edx (high).
    void store_pair(const Operand& operand) {
        switch (operand.kind) {
            case OperandKind::slot:
                op_ebp(0x89, kEax, slot_disp(operand.value));
                op_ebp(0x89, kEdx, slot_disp(operand.value) + 4);
                break;
            case OperandKind::global:
                op_ebx(0x89, kEax, global_disp(operand.value));
                op_ebx(0x89, kEdx, global_disp(operand.value) + 4);
                break;
            case OperandKind::indirect:
                op_ebp(0x8B, kEcx, slot_disp(operand.value));
                op_reg(0x89, kEax, kEcx);
                op_reg(0x89, kEdx, kEcx, 4);
                break;
            case OperandKind::immediate:
            case OperandKind::literal:
            case OperandKind::none:
                break;
        }
    }

    // ----- SSE2 and x87 ------------------------------------------------------

    // movsd xmm, operand
    void load_double(const int xmm, const Operand& operand) {
        switch (operand.kind) {
            case OperandKind::slot:
                bytes({0xF2, 0x0F, 0x10});
                byte(static_cast<std::uint8_t>(0x80 | (xmm << 3) | 5));
                dword(static_cast<std::uint32_t>(slot_disp(operand.value)));
                break;
            case OperandKind::global:
                bytes({0xF2, 0x0F, 0x10});
                byte(static_cast<std::uint8_t>(0x80 | (xmm << 3) | 3));
                dword(static_cast<std::uint32_t>(global_disp(operand.value)));
                break;
            case OperandKind::indirect:
                op_ebp(0x8B, kEcx, slot_disp(operand.value));
                bytes({0xF2, 0x0F, 0x10});
                byte(static_cast<std::uint8_t>((xmm << 3) | kEcx));
                break;
            case OperandKind::immediate: {
                // Through the stack: push high, push low, load, pop.
                const auto bits = static_cast<std::uint64_t>(operand.value);
                byte(0x68);
                dword(static_cast<std::uint32_t>(bits >> 32U));
                byte(0x68);
                dword(static_cast<std::uint32_t>(bits));
                bytes({0xF2, 0x0F, 0x10});
                byte(static_cast<std::uint8_t>((xmm << 3) | 4));
                byte(0x24);                 // [esp]
                bytes({0x83, 0xC4, 0x08});  // add esp, 8
                break;
            }
            case OperandKind::literal:
            case OperandKind::none:
                break;
        }
    }

    // movsd operand, xmm
    void store_double(const Operand& operand, const int xmm) {
        switch (operand.kind) {
            case OperandKind::slot:
                bytes({0xF2, 0x0F, 0x11});
                byte(static_cast<std::uint8_t>(0x80 | (xmm << 3) | 5));
                dword(static_cast<std::uint32_t>(slot_disp(operand.value)));
                break;
            case OperandKind::global:
                bytes({0xF2, 0x0F, 0x11});
                byte(static_cast<std::uint8_t>(0x80 | (xmm << 3) | 3));
                dword(static_cast<std::uint32_t>(global_disp(operand.value)));
                break;
            case OperandKind::indirect:
                op_ebp(0x8B, kEcx, slot_disp(operand.value));
                bytes({0xF2, 0x0F, 0x11});
                byte(static_cast<std::uint8_t>((xmm << 3) | kEcx));
                break;
            case OperandKind::immediate:
            case OperandKind::literal:
            case OperandKind::none:
                break;
        }
    }

    // A cdecl function returned a double in ST0: pop it into the operand.
    void store_st0(const Operand& operand) {
        switch (operand.kind) {
            case OperandKind::slot:
                bytes({0xDD, 0x9D});  // fstp qword [ebp + disp32]
                dword(static_cast<std::uint32_t>(slot_disp(operand.value)));
                break;
            case OperandKind::global:
                bytes({0xDD, 0x9B});  // fstp qword [ebx + disp32]
                dword(static_cast<std::uint32_t>(global_disp(operand.value)));
                break;
            case OperandKind::indirect:
                op_ebp(0x8B, kEcx, slot_disp(operand.value));
                bytes({0xDD, 0x19});  // fstp qword [ecx]
                break;
            case OperandKind::immediate:
            case OperandKind::literal:
            case OperandKind::none:
                break;
        }
    }

    // After a result in xmm0: error 6 when it is infinite or not a number.
    void check_finite() {
        bytes({0x83, 0xEC, 0x08});              // sub esp, 8
        bytes({0xF2, 0x0F, 0x11, 0x04, 0x24});  // movsd [esp], xmm0
        bytes({0x8B, 0x44, 0x24, 0x04});        // mov eax, [esp + 4]
        bytes({0x83, 0xC4, 0x08});              // add esp, 8
        byte(0x25);                             // and eax, 0x7FF00000
        dword(0x7FF00000U);
        byte(0x3D);  // cmp eax, 0x7FF00000
        dword(0x7FF00000U);
        jcc_overflow(kCcEqual);
    }

    // The result of a call or a service.
    void store_result(const ir::Instruction& instruction) {
        if (instruction.dst.kind == OperandKind::none) {
            if (instruction.double_result) {
                bytes({0xDD, 0xD8});  // fstp st0: drop the result
            }
            return;
        }
        if (instruction.double_result) {
            store_st0(instruction.dst);
        } else if (instruction.wide) {
            bytes({0x33, 0xD2});  // xor edx, edx: a handle's high half
            store_pair(instruction.dst);
        } else {
            store32(instruction.dst, kEax);
        }
    }

    // ----- Labels and jumps ------------------------------------------------

    struct FunctionState final {
        std::vector<std::int64_t> label_positions;
        std::vector<Fixup> fixups;
        std::vector<Fixup> overflow_jumps;
        std::vector<Fixup> zero_jumps;
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
        const std::int32_t frame =
            static_cast<std::int32_t>((8 + 8 * function.slot_count + 15) & ~15);
        // prologue
        byte(0x55);           // push ebp
        bytes({0x8B, 0xEC});  // mov ebp, esp
        bytes({0x81, 0xEC});  // sub esp, frame
        dword(static_cast<std::uint32_t>(frame));
        bytes({0x89, 0x5D, 0xFC});  // mov [ebp - 4], ebx
        // ebx = the address of the image
        const std::size_t call_at = image_.size();
        bytes({0xE8, 0x00, 0x00, 0x00, 0x00});  // call $ + 5
        byte(0x5B);                             // pop ebx
        bytes({0x81, 0xEB});                    // sub ebx, position
        dword(static_cast<std::uint32_t>(call_at + 5U));
        // Parameters: cdecl arguments from [ebp + 8], four bytes each and
        // eight for a double.
        std::int32_t offset = 8;
        for (std::size_t i = 0; i < function.parameters.size(); ++i) {
            const auto slot = static_cast<std::int64_t>(i);
            op_ebp(0x8B, kEax, offset);
            op_ebp(0x89, kEax, slot_disp(slot));
            if (function.parameters[i].is_double) {
                op_ebp(0x8B, kEax, offset + 4);
                op_ebp(0x89, kEax, slot_disp(slot) + 4);
                offset += 8;
            } else {
                // A pointer or a handle: the high half of the slot is zero.
                op_ebp(0xC7, 0, slot_disp(slot) + 4);
                dword(0);
                offset += 4;
            }
        }
        for (const auto& instruction : function.code) {
            emit_instruction(function, instruction, call_fixups);
        }
        // Every function ends with `ret`; the stubs follow it.
        emit_stubs(function);
        patch_labels();
    }

    void emit_stubs(const ir::Function& function) {
        if (!state_.overflow_jumps.empty()) {
            const std::size_t here = image_.size();
            for (const auto& jump : state_.overflow_jumps) {
                patch32(jump.at,
                        static_cast<std::int32_t>(here - (jump.at + 4U)));
            }
            emit_error_call(6, function);
        }
        if (!state_.zero_jumps.empty()) {
            const std::size_t here = image_.size();
            for (const auto& jump : state_.zero_jumps) {
                patch32(jump.at,
                        static_cast<std::int32_t>(here - (jump.at + 4U)));
            }
            emit_error_call(11, function);
        }
    }

    // Raises the error and goes to where this function handles errors.
    void emit_error_call(const int number, const ir::Function& function) {
        byte(0x68);  // push number
        dword(static_cast<std::uint32_t>(number));
        call_service(ir::Service::raise_error);
        bytes({0x83, 0xC4, 0x04});  // add esp, 4
        jmp(function.error_label);
    }

    void call_service(const ir::Service service) {
        bytes({0xFF, 0x93});  // call [ebx + disp32]
        dword(
            static_cast<std::uint32_t>(static_cast<std::size_t>(service) * 4U));
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

    // Pushes the arguments right to left; returns the bytes pushed.
    std::int32_t push_arguments(const ir::Instruction& instruction) {
        const auto& args = instruction.args;
        std::int32_t pushed = 0;
        for (std::size_t n = args.size(); n > 0; --n) {
            const std::size_t i = n - 1U;
            const Operand& operand = args[i];
            const bool is_double = ((instruction.double_args >> i) & 1U) != 0U;
            if (!is_double) {
                load32(kEax, operand);
                byte(0x50);  // push eax
                pushed += 4;
                continue;
            }
            pushed += 8;
            switch (operand.kind) {
                case OperandKind::slot:
                    bytes({0xFF, 0xB5});  // push dword [ebp + disp32 + 4]
                    dword(static_cast<std::uint32_t>(slot_disp(operand.value) +
                                                     4));
                    bytes({0xFF, 0xB5});
                    dword(static_cast<std::uint32_t>(slot_disp(operand.value)));
                    break;
                case OperandKind::global:
                    bytes({0xFF, 0xB3});  // push dword [ebx + disp32 + 4]
                    dword(static_cast<std::uint32_t>(
                        global_disp(operand.value) + 4));
                    bytes({0xFF, 0xB3});
                    dword(
                        static_cast<std::uint32_t>(global_disp(operand.value)));
                    break;
                case OperandKind::indirect:
                    op_ebp(0x8B, kEcx, slot_disp(operand.value));
                    bytes({0xFF, 0x71, 0x04});  // push dword [ecx + 4]
                    bytes({0xFF, 0x31});        // push dword [ecx]
                    break;
                case OperandKind::immediate: {
                    const auto bits = static_cast<std::uint64_t>(operand.value);
                    byte(0x68);
                    dword(static_cast<std::uint32_t>(bits >> 32U));
                    byte(0x68);
                    dword(static_cast<std::uint32_t>(bits));
                    break;
                }
                case OperandKind::literal:
                case OperandKind::none:
                    break;
            }
        }
        return pushed;
    }

    void drop_arguments(const std::int32_t pushed) {
        if (pushed == 0) {
            return;
        }
        bytes({0x81, 0xC4});  // add esp, pushed
        dword(static_cast<std::uint32_t>(pushed));
    }

    void emit_instruction(
        const ir::Function& function, const ir::Instruction& instruction,
        std::vector<std::pair<std::size_t, int>>& call_fixups) {
        switch (instruction.op) {
            case Opcode::move:
                if (instruction.wide) {
                    load_pair(instruction.a);
                    store_pair(instruction.dst);
                } else {
                    load32(kEax, instruction.a);
                    store32(instruction.dst, kEax);
                }
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
            case Opcode::fadd:
            case Opcode::fsub:
            case Opcode::fmul:
            case Opcode::fdivide:
                load_double(0, instruction.a);
                load_double(1, instruction.b);
                if (instruction.op == Opcode::fdivide) {
                    bytes({0x83, 0xEC, 0x08});              // sub esp, 8
                    bytes({0xF2, 0x0F, 0x11, 0x0C, 0x24});  // movsd [esp], xmm1
                    bytes({0x8B, 0x44, 0x24, 0x04});  // mov eax, [esp + 4]
                    bytes({0x8B, 0x14, 0x24});        // mov edx, [esp]
                    bytes({0x83, 0xC4, 0x08});        // add esp, 8
                    byte(0x25);                       // and eax, 0x7FFFFFFF
                    dword(0x7FFFFFFFU);
                    bytes({0x0B, 0xC2});  // or eax, edx
                    jcc_zero_divisor();
                }
                bytes({0xF2, 0x0F,
                       instruction.op == Opcode::fadd   ? std::uint8_t{0x58}
                       : instruction.op == Opcode::fsub ? std::uint8_t{0x5C}
                       : instruction.op == Opcode::fmul ? std::uint8_t{0x59}
                                                        : std::uint8_t{0x5E},
                       0xC1});
                check_finite();
                store_double(instruction.dst, 0);
                break;
            case Opcode::fnegate:
                load_pair(instruction.a);
                bytes({0x81, 0xF2});  // xor edx, 0x80000000
                dword(0x80000000U);
                store_pair(instruction.dst);
                break;
            case Opcode::fcompare: {
                load_double(0, instruction.a);
                load_double(1, instruction.b);
                bytes({0x66, 0x0F, 0x2E, 0xC1});  // ucomisd xmm0, xmm1
                std::uint8_t cc = kCcEqual;
                switch (instruction.condition) {
                    case Condition::equal:
                        cc = kCcEqual;
                        break;
                    case Condition::not_equal:
                        cc = kCcNotEqual;
                        break;
                    case Condition::less:
                        cc = 0x2;  // below
                        break;
                    case Condition::less_equal:
                        cc = 0x6;  // below or equal
                        break;
                    case Condition::greater:
                        cc = 0x7;  // above
                        break;
                    case Condition::greater_equal:
                        cc = 0x3;  // above or equal
                        break;
                }
                bytes({0x0F, static_cast<std::uint8_t>(0x90 | cc), 0xC0});
                bytes({0x0F, 0xB6, 0xC0});  // movzx eax, al
                bytes({0xF7, 0xD8});        // neg eax
                store32(instruction.dst, kEax);
                break;
            }
            case Opcode::int_to_double:
                load32(kEax, instruction.a);
                bytes({0xF2, 0x0F, 0x2A, 0xC0});  // cvtsi2sd xmm0, eax
                store_double(instruction.dst, 0);
                break;
            case Opcode::round_single:
                load_double(0, instruction.a);
                bytes({0xF2, 0x0F, 0x5A, 0xC0});  // cvtsd2ss xmm0, xmm0
                bytes({0xF3, 0x0F, 0x5A, 0xC0});  // cvtss2sd xmm0, xmm0
                check_finite();
                store_double(instruction.dst, 0);
                break;
            case Opcode::narrow:
                load32(kEax, instruction.a);
                check_result(Check::int16);
                store32(instruction.dst, kEax);
                break;
            case Opcode::address:
                if (instruction.a.kind == OperandKind::global) {
                    op_ebx(0x8D, kEax, global_disp(instruction.a.value));
                } else {
                    op_ebp(0x8D, kEax, slot_disp(instruction.a.value));
                }
                bytes({0x33, 0xD2});  // xor edx, edx
                store_pair(instruction.dst);
                break;
            case Opcode::pointer_add:
                load32(kEax, instruction.a);
                byte(0x05);  // add eax, imm32
                dword(static_cast<std::uint32_t>(
                    static_cast<std::int32_t>(instruction.b.value)));
                bytes({0x33, 0xD2});  // xor edx, edx
                store_pair(instruction.dst);
                break;
            case Opcode::check_error:
                bytes({0x83, 0xBB});  // cmp dword [ebx + flag], 0
                dword(static_cast<std::uint32_t>(flag_offset_));
                byte(0x00);
                jcc(kCcNotEqual, instruction.target);
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
            case Opcode::call: {
                const auto pushed = push_arguments(instruction);
                byte(0xE8);  // call rel32
                call_fixups.emplace_back(image_.size(), instruction.target);
                dword(0);
                drop_arguments(pushed);
                store_result(instruction);
                break;
            }
            case Opcode::runtime: {
                const auto pushed = push_arguments(instruction);
                call_service(static_cast<ir::Service>(instruction.target));
                drop_arguments(pushed);
                store_result(instruction);
                break;
            }
            case Opcode::print_text: {
                const auto index = static_cast<std::size_t>(instruction.target);
                byte(0x68);  // push length
                dword(
                    static_cast<std::uint32_t>(program_.strings[index].size()));
                op_ebx(0x8D, kEax,
                       static_cast<std::int32_t>(string_offsets_[index] + 8U));
                byte(0x50);  // push eax: the bytes of the literal
                call_service(ir::Service::print_text);
                bytes({0x83, 0xC4, 0x08});  // add esp, 8
                break;
            }
            case Opcode::ret:
                if (function.result_slot >= 0) {
                    if (function.result_double) {
                        bytes({0xDD, 0x85});  // fld qword [ebp + disp32]
                        dword(static_cast<std::uint32_t>(
                            slot_disp(function.result_slot)));
                    } else {
                        load32(kEax, ir::slot(function.result_slot));
                    }
                }
                bytes({0x8B, 0x5D, 0xFC});  // mov ebx, [ebp - 4]
                byte(0xC9);                 // leave
                byte(0xC3);                 // ret
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

    const ir::Program& program_;
    std::vector<std::uint8_t> image_;
    std::size_t flag_offset_{};
    std::size_t globals_offset_{};
    std::vector<std::size_t> string_offsets_;
    FunctionState state_;
};

}  // namespace

BackEndResult generate_x86(const ir::Program& program) {
    Emitter emitter(program);
    return emitter.run();
}

}  // namespace wfc::compiler
