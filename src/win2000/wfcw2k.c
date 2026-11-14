/* The small runtime of a compiled console program that runs on Windows 2000 and
 * later (wfcw2k.exe).
 *
 * `wfc build --target win2000` appends the program (the machine code of the x86
 * back end, packaged as the other programs of WFC are) to a copy of this
 * executable. It is written in C and built with TinyCC against WCRT, with
 * WCRT's console startup object, so it needs no C++ runtime and imports only
 * KERNEL32 functions that Windows 2000 has.
 *
 * It reads its own payload, loads the image of the program into executable
 * memory, fills the entry table with the runtime services below and calls the
 * program. The services are the ones of src/compiler/native_host.cpp that a
 * console program needs: Print, strings, arrays, doubles and math, and the
 * error state. A program that uses another service (Variants, classes,
 * forms, files) is refused at build time.
 *
 * Strings are bytes here; the full runtime counts non-ASCII text in UTF-16
 * units.
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----- The few Win32 functions used (all in KERNEL32 of Windows 2000) ------
 */

#if defined(__TINYC__)
#define WFC_STDCALL __attribute__((stdcall))
#else
#define WFC_STDCALL __stdcall
#endif
#define IMPORT __declspec(dllimport)

typedef unsigned long DWORD;
typedef int BOOL;
typedef void* HANDLE;

typedef struct {
    void* BaseAddress;
    void* AllocationBase;
    DWORD AllocationProtect;
    size_t RegionSize;
    DWORD State;
    DWORD Protect;
    DWORD Type;
} MEMORY_BASIC_INFORMATION;

IMPORT DWORD WFC_STDCALL GetModuleFileNameA(HANDLE module, char* name,
                                            DWORD size);
IMPORT HANDLE WFC_STDCALL CreateFileA(const char* name, DWORD access,
                                      DWORD share, void* security,
                                      DWORD disposition, DWORD flags,
                                      HANDLE template_file);
IMPORT DWORD WFC_STDCALL GetFileSize(HANDLE file, DWORD* high);
IMPORT BOOL WFC_STDCALL ReadFile(HANDLE file, void* buffer, DWORD count,
                                 DWORD* done, void* overlapped);
IMPORT BOOL WFC_STDCALL CloseHandle(HANDLE handle);
IMPORT void* WFC_STDCALL VirtualAlloc(void* where, size_t size, DWORD type,
                                      DWORD protect);
IMPORT BOOL WFC_STDCALL VirtualProtect(void* where, size_t size, DWORD protect,
                                       DWORD* previous);
IMPORT size_t WFC_STDCALL VirtualQuery(const void* where,
                                       MEMORY_BASIC_INFORMATION* info,
                                       size_t size);
IMPORT HANDLE WFC_STDCALL GetCurrentProcess(void);
IMPORT BOOL WFC_STDCALL FlushInstructionCache(HANDLE process, const void* where,
                                              size_t size);

#define GENERIC_READ_ 0x80000000UL
#define FILE_SHARE_READ_ 1UL
#define OPEN_EXISTING_ 3UL
#define MEM_COMMIT_RESERVE_ 0x3000UL
#define PAGE_READWRITE_ 0x04UL
#define PAGE_EXECUTE_READWRITE_ 0x40UL

/* ----- Strings ---------------------------------------------------------------
 *
 * A handle points to an RtString; the null handle is the empty string. A
 * literal of the image has the same layout with a count of -1. */

typedef struct RtString {
    int32_t refs;
    int32_t length;
    char data[1];
} RtString;

#define OMITTED INT32_MIN /* an omitted Mid length */

static int32_t length_of(const RtString* text) {
    return text == NULL ? 0 : text->length;
}

static void die(const char* message) {
    fflush(stdout);
    fprintf(stderr, "%s\n", message);
    exit(1);
}

static RtString* make_string(const char* data, size_t size) {
    RtString* text;
    if (size == 0U) {
        return NULL;
    }
    text = (RtString*)malloc(offsetof(RtString, data) + size + 1U);
    if (text == NULL) {
        die("out of memory");
    }
    text->refs = 1;
    text->length = (int32_t)size;
    memcpy(text->data, data, size);
    text->data[size] = '\0';
    return text;
}

static RtString* make_cstring(const char* text) {
    return make_string(text, strlen(text));
}

static RtString* retain(RtString* text) {
    if (text != NULL && text->refs > 0) {
        ++text->refs;
    }
    return text;
}

static void release(RtString* text) {
    if (text != NULL && text->refs > 0 && --text->refs == 0) {
        free(text);
    }
}

/* ----- Output ---------------------------------------------------------------
 */

static int g_column = 0; /* the column the next character lands in */

static void put(const char* text, size_t length) {
    size_t i;
    fwrite(text, 1U, length, stdout);
    for (i = 0; i < length; ++i) {
        g_column = text[i] == '\n' ? 0 : g_column + 1;
    }
}

static void put_blanks(int count) {
    while (count-- > 0) {
        put(" ", 1U);
    }
}

static void put_cstring(const char* text) {
    put(text, strlen(text));
}

/* ----- The error state -------------------------------------------------------
 */

static const char* standard_error_text(int32_t number) {
    switch (number) {
        case 5:
            return "Invalid procedure call or argument";
        case 6:
            return "Overflow";
        case 7:
            return "Out of memory";
        case 9:
            return "Subscript out of range";
        case 11:
            return "Division by zero";
        case 13:
            return "Type mismatch";
        case 28:
            return "Out of stack space";
        case 20:
            return "Resume without error";
        case 35:
            return "Sub or Function not defined";
        case 52:
            return "Bad file name or number";
        case 53:
            return "File not found";
        case 54:
            return "Bad file mode";
        case 55:
            return "File already open";
        case 58:
            return "File already exists";
        case 62:
            return "Input past end of file";
        case 70:
            return "Permission denied";
        case 75:
            return "Path/File access error";
        case 76:
            return "Path not found";
        case 91:
            return "Object variable or With block variable not set";
        case 92:
            return "For loop not initialized";
        case 94:
            return "Invalid use of Null";
        case 429:
            return "ActiveX component can't create object";
        case 438:
            return "Object doesn't support this property or method";
        case 445:
            return "Object doesn't support this action";
        case 448:
            return "Named argument not found";
        case 449:
            return "Argument not optional";
        case 450:
            return "Wrong number of arguments or invalid property assignment";
        case 451:
            return "Property let procedure not defined and property get "
                   "procedure did not return an object";
        case 457:
            return "This key is already associated with an element of this "
                   "collection";
        default:
            return "Application-defined or object-defined error";
    }
}

#define TEXT_MAX 512

static int32_t* g_pending; /* the pending-error flag inside the image */
static int32_t g_error_number;
static char g_error_description[TEXT_MAX];
static char g_error_source[TEXT_MAX];
static char g_help_file[TEXT_MAX];
static int32_t g_help_context;

static void copy_text(char* into, const char* from, size_t size) {
    strncpy(into, from, size - 1U);
    into[size - 1U] = '\0';
}

/* Records a run-time error and marks it pending; generated code sees the flag
 * and goes to the procedure's handler. */
static void raise_error(int32_t number) {
    g_error_number = number;
    copy_text(g_error_description, standard_error_text(number), TEXT_MAX);
    copy_text(g_error_source, "Project1", TEXT_MAX);
    g_help_file[0] = '\0';
    g_help_context = 0;
    if (g_pending != NULL) {
        *g_pending = 1;
    }
}

static void finish_output(void) {
    fflush(stdout);
}

static void end_program(void) {
    finish_output();
    exit(0);
}

static void unsupported(void) {
    finish_output();
    fputs(
        "wfc: this program needs a part of the WFC runtime that the small "
        "runtime for Windows 2000 does not have\n",
        stderr);
    exit(1);
}

/* ----- Print
 * ------------------------------------------------------------------ */

static void print_number(int32_t value) {
    char buffer[24];
    sprintf(buffer, "%s%ld ", value >= 0 ? " " : "", (long)value);
    put_cstring(buffer);
}

static void print_boolean(int32_t value) {
    put_cstring(value != 0 ? "True" : "False");
}

static void print_text(const char* text, int32_t length) {
    put(text, (size_t)length);
}

static void print_newline(void) {
    put("\n", 1U);
}

/* Tab(n): to column n (a new line first when the column has passed). */
static void print_tab(int32_t column) {
    int32_t at = (int32_t)g_column + 1;
    if (column < at) {
        put("\n", 1U);
        put_blanks(column > 1 ? column - 1 : 0);
    } else {
        put_blanks(column - at);
    }
}

static void print_spc(int32_t count) {
    if (count > 0) {
        put_blanks(count);
    }
}

static void print_zone(void) {
    put_blanks(14 - g_column % 14);
}

static void print_string(const RtString* text) {
    if (text != NULL) {
        put(text->data, (size_t)text->length);
    }
}

/* ----- String services -------------------------------------------------------
 */

static RtString* string_concat(const RtString* a, const RtString* b) {
    RtString* joined;
    if (length_of(a) == 0) {
        return retain((RtString*)b);
    }
    if (length_of(b) == 0) {
        return retain((RtString*)a);
    }
    joined = (RtString*)malloc(offsetof(RtString, data) + (size_t)a->length +
                               (size_t)b->length + 1U);
    if (joined == NULL) {
        die("out of memory");
    }
    joined->refs = 1;
    joined->length = a->length + b->length;
    memcpy(joined->data, a->data, (size_t)a->length);
    memcpy(joined->data + a->length, b->data, (size_t)b->length);
    joined->data[joined->length] = '\0';
    return joined;
}

static void string_assign(RtString** variable, RtString* value) {
    retain(value);
    release(*variable);
    *variable = value;
}

static void string_release(RtString* text) {
    release(text);
}

static void string_addref(RtString* text) {
    retain(text);
}

static int32_t string_length(const RtString* text) {
    return length_of(text);
}

static int32_t string_compare(const RtString* a, const RtString* b) {
    int32_t left = length_of(a);
    int32_t right = length_of(b);
    int32_t common = left < right ? left : right;
    int order = common == 0 ? 0 : memcmp(a->data, b->data, (size_t)common);
    if (order == 0) {
        order = left < right ? -1 : (left > right ? 1 : 0);
    }
    return order < 0 ? -1 : (order > 0 ? 1 : 0);
}

static RtString* string_fixed(const RtString* text, int32_t length) {
    RtString* result;
    int32_t have = length_of(text);
    int32_t i;
    if (length <= 0) {
        return NULL;
    }
    result = (RtString*)malloc(offsetof(RtString, data) + (size_t)length + 1U);
    if (result == NULL) {
        die("out of memory");
    }
    result->refs = 1;
    result->length = length;
    for (i = 0; i < length; ++i) {
        result->data[i] = i < have ? text->data[i] : ' ';
    }
    result->data[length] = '\0';
    return result;
}

static void string_mid_assign(RtString** variable, int32_t start,
                              int32_t length, const RtString* text) {
    int32_t target = length_of(*variable);
    int32_t begin = start - 1;
    int32_t count;
    RtString* replaced;
    if (start < 1 || length < -1) {
        raise_error(5);
        return;
    }
    if (begin >= target) {
        return;
    }
    count = length_of(text);
    if (count > target - begin) {
        count = target - begin;
    }
    if (length >= 0 && count > length) {
        count = length;
    }
    replaced = make_string((*variable)->data, (size_t)target);
    memcpy(replaced->data + begin, text != NULL ? text->data : "",
           (size_t)count);
    release(*variable);
    *variable = replaced;
}

static RtString* string_left(RtString* text, int32_t count) {
    if (count < 0) {
        raise_error(5);
        return NULL;
    }
    if (count >= length_of(text)) {
        return retain(text);
    }
    return make_string(text->data, (size_t)count);
}

static RtString* string_right(RtString* text, int32_t count) {
    if (count < 0) {
        raise_error(5);
        return NULL;
    }
    if (count >= length_of(text)) {
        return retain(text);
    }
    return make_string(text->data + (text->length - count), (size_t)count);
}

static RtString* string_mid(RtString* text, int32_t start, int32_t count) {
    size_t rest;
    size_t wanted;
    if (start < 1 || (count < 0 && count != OMITTED)) {
        raise_error(5);
        return NULL;
    }
    if (start > length_of(text)) {
        return NULL;
    }
    rest = (size_t)(text->length - (start - 1));
    wanted =
        count == OMITTED ? rest : ((size_t)count < rest ? (size_t)count : rest);
    if (start == 1 && wanted == (size_t)text->length) {
        return retain(text);
    }
    return make_string(text->data + (start - 1), wanted);
}

static RtString* string_upper(const RtString* text) {
    RtString* result =
        make_string(text != NULL ? text->data : "", (size_t)length_of(text));
    int32_t i;
    for (i = 0; i < length_of(result); ++i) {
        char c = result->data[i];
        if (c >= 'a' && c <= 'z') {
            result->data[i] = (char)(c - 'a' + 'A');
        }
    }
    return result;
}

static RtString* string_lower(const RtString* text) {
    RtString* result =
        make_string(text != NULL ? text->data : "", (size_t)length_of(text));
    int32_t i;
    for (i = 0; i < length_of(result); ++i) {
        char c = result->data[i];
        if (c >= 'A' && c <= 'Z') {
            result->data[i] = (char)(c - 'A' + 'a');
        }
    }
    return result;
}

static RtString* trimmed(const RtString* text, int left, int right) {
    int32_t begin = 0;
    int32_t end = length_of(text);
    while (left && begin < end && text->data[begin] == ' ') {
        ++begin;
    }
    while (right && end > begin && text->data[end - 1] == ' ') {
        --end;
    }
    return make_string(text != NULL ? text->data + begin : "",
                       (size_t)(end - begin));
}

static RtString* string_trim(const RtString* text) {
    return trimmed(text, 1, 1);
}

static RtString* string_ltrim(const RtString* text) {
    return trimmed(text, 1, 0);
}

static RtString* string_rtrim(const RtString* text) {
    return trimmed(text, 0, 1);
}

static RtString* string_chr(int32_t code) {
    char c;
    if (code < 0 || code > 255) {
        raise_error(5);
        return NULL;
    }
    c = (char)code;
    return make_string(&c, 1U);
}

static int32_t string_asc(const RtString* text) {
    if (length_of(text) == 0) {
        raise_error(5);
        return 0;
    }
    return (unsigned char)text->data[0];
}

static RtString* string_from_number(int32_t value) {
    char buffer[24];
    sprintf(buffer, "%s%ld", value >= 0 ? " " : "", (long)value);
    return make_cstring(buffer);
}

static RtString* string_from_integer(int32_t value) {
    char buffer[24];
    sprintf(buffer, "%ld", (long)value);
    return make_cstring(buffer);
}

static RtString* string_from_boolean(int32_t value) {
    return make_cstring(value != 0 ? "True" : "False");
}

static int32_t string_instr(int32_t start, const RtString* text,
                            const RtString* pattern) {
    int32_t have = length_of(text);
    int32_t want = length_of(pattern);
    int32_t at;
    if (start < 1) {
        raise_error(5);
        return 0;
    }
    if (start > have) {
        return 0;
    }
    if (want == 0) {
        return start;
    }
    for (at = start - 1; at + want <= have; ++at) {
        if (memcmp(text->data + at, pattern->data, (size_t)want) == 0) {
            return at + 1;
        }
    }
    return 0;
}

static RtString* string_space(int32_t count) {
    RtString* result;
    if (count < 0) {
        raise_error(5);
        return NULL;
    }
    result = string_fixed(NULL, count);
    return result;
}

/* ----- Doubles and math
 * ------------------------------------------------------- */

/* The digits of a positive finite Double, correctly rounded to `count`
 * significant digits (the library's printf rounds a decimal expansion of
 * limited length, which is a unit off for some values). The value is
 * mantissa * 2^exp2; the exact decimal expansion is made with a number in
 * base 10^9. `digits` receives `count` characters and the result is the
 * decimal exponent of the first digit (value = d.ddd * 10^result). */
#define BIG_LIMBS 120

static int exact_digits(double value, int count, char* digits) {
    uint32_t limb[BIG_LIMBS];
    int used = 1;
    int exp2;
    int shift = 0; /* the value is the big number / 10^shift */
    uint64_t mantissa;
    union {
        double d;
        uint64_t u;
    } bits;
    char text[BIG_LIMBS * 9 + 2];
    int length = 0;
    int i;
    int k;
    int first_digit_exponent;
    bits.d = value;
    exp2 = (int)((bits.u >> 52) & 0x7FF);
    mantissa = bits.u & 0xFFFFFFFFFFFFFULL;
    if (exp2 == 0) {
        exp2 = -1074;
    } else {
        mantissa |= 1ULL << 52;
        exp2 -= 1075;
    }
    memset(limb, 0, sizeof(limb));
    limb[0] = (uint32_t)(mantissa % 1000000000ULL);
    limb[1] = (uint32_t)((mantissa / 1000000000ULL) % 1000000000ULL);
    limb[2] = (uint32_t)(mantissa / 1000000000000000000ULL);
    used = limb[2] != 0U ? 3 : (limb[1] != 0U ? 2 : 1);
    {
        /* Multiply by 2^exp2 (exp2 >= 0) or by 5^-exp2 (the rest is a power
         * of ten that moves the decimal point). */
        int times = exp2 >= 0 ? exp2 : -exp2;
        uint32_t factor = exp2 >= 0 ? 2U : 5U;
        if (exp2 < 0) {
            shift = -exp2;
        }
        while (times > 0) {
            /* up to 2^13 or 5^9 at once keeps the carries inside 64 bits */
            int step = exp2 >= 0 ? (times > 29 ? 29 : times)
                                 : (times > 12 ? 12 : times);
            uint64_t multiplier = 1U;
            uint64_t carry = 0U;
            for (k = 0; k < step; ++k) {
                multiplier *= factor;
            }
            for (i = 0; i < used; ++i) {
                uint64_t product = (uint64_t)limb[i] * multiplier + carry;
                limb[i] = (uint32_t)(product % 1000000000ULL);
                carry = product / 1000000000ULL;
            }
            while (carry != 0U && used < BIG_LIMBS) {
                limb[used++] = (uint32_t)(carry % 1000000000ULL);
                carry /= 1000000000ULL;
            }
            times -= step;
        }
    }
    for (i = used - 1; i >= 0; --i) {
        char piece[16];
        sprintf(piece, i == used - 1 ? "%lu" : "%09lu", (unsigned long)limb[i]);
        memcpy(text + length, piece, strlen(piece));
        length += (int)strlen(piece);
    }
    text[length] = '\0';
    /* value = text * 10^-shift: the first digit has exponent length-1-shift */
    first_digit_exponent = length - 1 - shift;
    if (length > count) {
        int round_up = 0;
        int next = text[count] - '0';
        int rest_nonzero = 0;
        for (i = count + 1; i < length; ++i) {
            if (text[i] != '0') {
                rest_nonzero = 1;
                break;
            }
        }
        if (next > 5 || (next == 5 && rest_nonzero)) {
            round_up = 1;
        } else if (next == 5) {
            round_up = ((text[count - 1] - '0') & 1) != 0; /* a tie: to even */
        }
        for (i = 0; i < count; ++i) {
            digits[i] = text[i];
        }
        if (round_up) {
            for (i = count - 1; i >= 0; --i) {
                if (digits[i] == '9') {
                    digits[i] = '0';
                } else {
                    ++digits[i];
                    break;
                }
            }
            if (i < 0) {
                /* 999... became 1000... */
                digits[0] = '1';
                for (k = 1; k < count; ++k) {
                    digits[k] = '0';
                }
                ++first_digit_exponent;
            }
        }
    } else {
        for (i = 0; i < count; ++i) {
            digits[i] = i < length ? text[i] : '0';
        }
    }
    return first_digit_exponent;
}

/* VB6 writes a Double with 15 significant digits and a Single with 7: the
 * exponent form from 1E15 up, and a number below 1 in full while the zeros
 * after the point and the significant digits fit the digits of the type;
 * negative zero is "0". */
static void format_float(char* buffer, size_t size, double value, int digits) {
    char significant[24];
    char text[48];
    size_t at = 0;
    int exponent;
    int i;
    int last;
    if (value != value) {
        copy_text(buffer, "nan", size);
        return;
    }
    if (value > 1.7976931348623157e308 || value < -1.7976931348623157e308) {
        copy_text(buffer, value > 0 ? "inf" : "-inf", size);
        return;
    }
    if (value == 0.0) {
        copy_text(buffer, "0", size);
        return;
    }
    if (digits < 1 || digits > 17) {
        digits = 15;
    }
    if (value < 0.0) {
        text[at++] = '-';
        value = -value;
    }
    exponent = exact_digits(value, digits, significant);
    last = digits - 1;
    while (last > 0 && significant[last] == '0') {
        --last; /* %G drops trailing zeros */
    }
    if (exponent >= digits ||
        (exponent < 0 && -exponent - 1 + last + 1 > digits)) {
        text[at++] = significant[0];
        if (last > 0) {
            text[at++] = '.';
            for (i = 1; i <= last; ++i) {
                text[at++] = significant[i];
            }
        }
        at += (size_t)sprintf(text + at, "E%c%02d", exponent < 0 ? '-' : '+',
                              exponent < 0 ? -exponent : exponent);
    } else if (exponent < 0) {
        text[at++] = '0';
        text[at++] = '.';
        for (i = 0; i < -exponent - 1; ++i) {
            text[at++] = '0';
        }
        for (i = 0; i <= last; ++i) {
            text[at++] = significant[i];
        }
    } else {
        for (i = 0; i <= exponent; ++i) {
            text[at++] = i < digits ? significant[i] : '0';
        }
        if (last > exponent) {
            text[at++] = '.';
            for (i = exponent + 1; i <= last; ++i) {
                text[at++] = significant[i];
            }
        }
    }
    text[at] = '\0';
    copy_text(buffer, text, size);
}

static void print_double(double value, int32_t digits) {
    char buffer[64];
    format_float(buffer, sizeof(buffer), value, (int)digits);
    if (value >= 0) {
        put(" ", 1U);
    }
    put_cstring(buffer);
    put(" ", 1U);
}

static RtString* string_from_double(double value, int32_t digits,
                                    int32_t sign_space) {
    char buffer[64];
    char joined[72];
    format_float(buffer, sizeof(buffer), value, (int)digits);
    sprintf(joined, "%s%s", sign_space != 0 && value >= 0 ? " " : "", buffer);
    return make_cstring(joined);
}

static int32_t double_to_long(double value) {
    double rounded = nearbyint(value); /* to even, as CInt and CLng */
    if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
        raise_error(6);
        return 0;
    }
    return (int32_t)rounded;
}

/* exp and pow, accurate to the last digit of a Double. WCRT's own are off by
 * one unit in the last place for some arguments, which shows in the 15 digits
 * VB prints. The sum is made in double-double arithmetic (two doubles); the
 * x87 unit of Windows rounds to 53 bits, which that arithmetic needs. */
typedef struct {
    double hi;
    double lo;
} DoubleDouble;

static DoubleDouble dd_quick_sum(double a, double b) {
    DoubleDouble r;
    r.hi = a + b;
    r.lo = b - (r.hi - a);
    return r;
}

static DoubleDouble dd_two_sum(double a, double b) {
    DoubleDouble r;
    double bb;
    r.hi = a + b;
    bb = r.hi - a;
    r.lo = (a - (r.hi - bb)) + (b - bb);
    return r;
}

static DoubleDouble dd_two_product(double a, double b) {
    DoubleDouble r;
    double t = 134217729.0 * a;
    double ah = t - (t - a);
    double al = a - ah;
    double bh;
    double bl;
    t = 134217729.0 * b;
    bh = t - (t - b);
    bl = b - bh;
    r.hi = a * b;
    r.lo = ((ah * bh - r.hi) + ah * bl + al * bh) + al * bl;
    return r;
}

static DoubleDouble dd_add(DoubleDouble a, DoubleDouble b) {
    DoubleDouble s = dd_two_sum(a.hi, b.hi);
    DoubleDouble t = dd_two_sum(a.lo, b.lo);
    s.lo += t.hi;
    s = dd_quick_sum(s.hi, s.lo);
    s.lo += t.lo;
    return dd_quick_sum(s.hi, s.lo);
}

static DoubleDouble dd_negate(DoubleDouble a) {
    a.hi = -a.hi;
    a.lo = -a.lo;
    return a;
}

static DoubleDouble dd_mul(DoubleDouble a, DoubleDouble b) {
    DoubleDouble p = dd_two_product(a.hi, b.hi);
    p.lo += a.hi * b.lo + a.lo * b.hi;
    return dd_quick_sum(p.hi, p.lo);
}

static DoubleDouble dd_divide_by(DoubleDouble a, double b) {
    double q1 = a.hi / b;
    DoubleDouble p = dd_two_product(q1, b);
    DoubleDouble r = dd_add(a, dd_negate(p));
    double q2 = r.hi / b;
    double q3;
    p = dd_two_product(q2, b);
    r = dd_add(r, dd_negate(p));
    q3 = r.hi / b;
    r = dd_quick_sum(q1, q2);
    return dd_add(r, dd_quick_sum(q3, 0.0));
}

static double accurate_exp(double x) {
    /* fdlibm's split of ln 2: the high part has trailing zero bits, so k times
     * it is exact. */
    const double ln2_high = 6.93147180369123816490e-01;
    const double ln2_low = 1.90821492927058770002e-10;
    double k = floor(x / 0.6931471805599453 + 0.5);
    DoubleDouble r = dd_two_sum(x - k * ln2_high, -k * ln2_low);
    DoubleDouble term;
    DoubleDouble sum;
    int n;
    if (x != x || x > 709.0 || x < -708.0) {
        return exp(x);
    }
    term.hi = 1.0;
    term.lo = 0.0;
    sum = term;
    for (n = 1; n < 30; ++n) {
        term = dd_divide_by(dd_mul(term, r), (double)n);
        sum = dd_add(sum, term);
        if (fabs(term.hi) < 1e-34) {
            break;
        }
    }
    return ldexp(sum.hi + sum.lo, (int)k);
}

/* exp of a double-double argument (|x| below about 700), accurate to about
 * 30 digits. */
static DoubleDouble dd_exp(DoubleDouble x) {
    static const DoubleDouble ln2 = {0.6931471805599453,
                                     2.3190468138462996e-17};
    double k = floor(x.hi / 0.6931471805599453 + 0.5);
    DoubleDouble multiple;
    DoubleDouble r;
    DoubleDouble term;
    DoubleDouble sum;
    DoubleDouble result;
    int n;
    multiple.hi = k;
    multiple.lo = 0.0;
    r = dd_add(x, dd_negate(dd_mul(multiple, ln2)));
    term.hi = 1.0;
    term.lo = 0.0;
    sum = term;
    for (n = 1; n < 40; ++n) {
        term = dd_divide_by(dd_mul(term, r), (double)n);
        sum = dd_add(sum, term);
        if (fabs(term.hi) < 1e-36) {
            break;
        }
    }
    result.hi = ldexp(sum.hi, (int)k);
    result.lo = ldexp(sum.lo, (int)k);
    return result;
}

/* ln of a positive double as a double-double: the library's log refined by
 * one Newton step on exp. */
static DoubleDouble dd_log(double x) {
    DoubleDouble y;
    DoubleDouble negative;
    DoubleDouble product;
    DoubleDouble one;
    DoubleDouble pair;
    y.hi = log(x);
    y.lo = 0.0;
    negative = dd_negate(y);
    product = dd_mul(dd_exp(negative), (DoubleDouble){x, 0.0});
    one.hi = 1.0;
    one.lo = 0.0;
    pair = dd_add(product, dd_negate(one)); /* x * exp(-y) - 1 */
    return dd_add(y, pair);
}

/* A square root correct to the last bit: WCRT's is a unit off for some
 * arguments, so its result is corrected once with the exact residual. */
static double accurate_sqrt(double x) {
    double s = sqrt(x);
    DoubleDouble square;
    double residual;
    if (!(x > 0.0) || s != s || s > 1.7976931348623157e308) {
        return s;
    }
    square = dd_two_product(s, s);
    residual = (x - square.hi) - square.lo;
    return s + residual / (2.0 * s);
}

static double accurate_pow(double base, double exponent) {
    if (exponent == 0.5 && base >= 0.0) {
        return accurate_sqrt(base);
    }
    if (base < 0.0 && exponent != floor(exponent)) {
        double zero = 0.0;
        return zero /
               zero; /* not a number: a negative base needs an integer power */
    }
    if (exponent > 0.0 && base != 0.0 && base == base &&
        exponent * log(fabs(base)) > 709.78) {
        /* too large for a Double */
        double zero = 0.0;
        double infinity = 1.0 / zero;
        return base < 0.0 && fmod(exponent, 2.0) != 0.0 ? -infinity : infinity;
    }
    if (exponent == floor(exponent) && fabs(exponent) <= 1024.0 &&
        base == base) {
        /* An integer power: repeated squaring in double-double. */
        DoubleDouble result;
        DoubleDouble square;
        long n = (long)fabs(exponent);
        result.hi = 1.0;
        result.lo = 0.0;
        square.hi = base;
        square.lo = 0.0;
        while (n > 0) {
            if ((n & 1L) != 0L) {
                result = dd_mul(result, square);
            }
            square = dd_mul(square, square);
            n >>= 1;
        }
        if (exponent < 0.0) {
            return 1.0 / (result.hi + result.lo);
        }
        return result.hi + result.lo;
    }
    if (base > 0.0 && base == base && exponent == exponent &&
        fabs(exponent) < 1e6 && fabs(exponent * log(base)) < 700.0) {
        DoubleDouble factor;
        DoubleDouble scaled;
        DoubleDouble value;
        factor.hi = exponent;
        factor.lo = 0.0;
        scaled = dd_mul(factor, dd_log(base));
        value = dd_exp(scaled);
        return value.hi + value.lo;
    }
    return pow(base, exponent);
}

static double math_function(int32_t which, double value) {
    double result;
    switch (which) {
        case 0:
            if (value < 0) {
                raise_error(5);
                return 0;
            }
            return accurate_sqrt(value);
        case 1:
            return sin(value);
        case 2:
            return cos(value);
        case 3:
            return tan(value);
        case 4:
            return atan(value);
        case 5:
            if (value <= 0) {
                raise_error(5);
                return 0;
            }
            return log(value);
        case 6:
            result = accurate_exp(value);
            if (result > 1.7976931348623157e308) {
                raise_error(6);
                return 0;
            }
            return result;
        case 7:
            return floor(value);
        case 8:
            return trunc(value);
        case 9:
            return fabs(value);
        default:
            return value;
    }
}

static double double_power(double base, double exponent) {
    double result;
    /* Measured under VB6: zero to a negative power and a negative number to a
     * fraction are error 5, an overflow is error 6. */
    if ((base == 0.0 && exponent < 0.0) ||
        (base < 0.0 && exponent != floor(exponent))) {
        raise_error(5);
        return 0.0;
    }
    result = accurate_pow(base, exponent);
    if (result != result || result > 1.7976931348623157e308 ||
        result < -1.7976931348623157e308) {
        raise_error(6);
        return 0.0;
    }
    return result;
}

static int32_t double_sign(double value) {
    return value > 0 ? 1 : (value < 0 ? -1 : 0);
}

/* ----- Arrays
 * ------------------------------------------------------------------
 *
 * Kind 0 holds 32-bit integers, kind 1 doubles, kind 2 string handles (eight
 * byte cells), kind 3 records whose String and array fields the descriptor
 * lists. The null handle is an array that has not been dimensioned. */

typedef struct {
    int32_t lower;
    int32_t upper;
} RtBound;

typedef struct RtArray {
    int32_t kind;
    int32_t dims;
    int32_t fixed;
    int32_t count;
    RtBound bounds[3];
    int32_t size; /* bytes per element */
    int32_t pad;
    const int32_t* strings; /* kind 3: a count, then the field offsets */
} RtArray;

typedef struct {
    RtString* handle;
    int32_t pad;
} HandleCell;

#define IN_RECORD_VARIANT 0x40000000
#define IN_RECORD_ARRAY 0x20000000
#define IN_RECORD_MASK (~0x60000000)

static char* data_of(RtArray* array) {
    return (char*)array + sizeof(RtArray);
}

static RtArray* array_clone(const RtArray* source);
static void array_release(RtArray* array);
struct RtVariant;
static struct RtVariant* variant_retain(struct RtVariant* handle);
static void variant_release(struct RtVariant* handle);

/* After a copy of the bytes of an array: its handles are shared. */
static void array_retain_elements(RtArray* array) {
    int32_t i;
    int32_t k;
    if (array->kind == 3 && array->strings != NULL) {
        for (i = 0; i < array->count; ++i) {
            char* base = data_of(array) + (size_t)i * (size_t)array->size;
            for (k = 0; k < array->strings[0]; ++k) {
                int32_t where = array->strings[1 + k];
                if ((where & IN_RECORD_VARIANT) != 0) {
                    variant_retain(
                        *(struct RtVariant**)(base + (where & IN_RECORD_MASK)));
                    continue;
                }
                if ((where & IN_RECORD_ARRAY) != 0) {
                    RtArray** handle =
                        (RtArray**)(base + (where & IN_RECORD_MASK));
                    *handle = array_clone(*handle);
                    continue;
                }
                retain(*(RtString**)(base + where));
            }
        }
    } else if (array->kind == 2) {
        HandleCell* cells = (HandleCell*)data_of(array);
        for (i = 0; i < array->count; ++i) {
            retain(cells[i].handle);
        }
    } else if (array->kind == 4) {
        HandleCell* cells = (HandleCell*)data_of(array);
        for (i = 0; i < array->count; ++i) {
            variant_retain((struct RtVariant*)cells[i].handle);
        }
    } else if (array->kind == 5) {
        unsupported();
    }
}

static void release_elements(RtArray* array) {
    int32_t i;
    int32_t k;
    if (array->kind == 3 && array->strings != NULL) {
        for (i = 0; i < array->count; ++i) {
            char* base = data_of(array) + (size_t)i * (size_t)array->size;
            for (k = 0; k < array->strings[0]; ++k) {
                int32_t where = array->strings[1 + k];
                if ((where & IN_RECORD_VARIANT) != 0) {
                    struct RtVariant** handle =
                        (struct RtVariant**)(base + (where & IN_RECORD_MASK));
                    variant_release(*handle);
                    *handle = NULL;
                    continue;
                }
                if ((where & IN_RECORD_ARRAY) != 0) {
                    RtArray** handle =
                        (RtArray**)(base + (where & IN_RECORD_MASK));
                    array_release(*handle);
                    *handle = NULL;
                    continue;
                }
                {
                    RtString** handle = (RtString**)(base + where);
                    release(*handle);
                    *handle = NULL;
                }
            }
        }
    } else if (array->kind == 2) {
        HandleCell* cells = (HandleCell*)data_of(array);
        for (i = 0; i < array->count; ++i) {
            release(cells[i].handle);
            cells[i].handle = NULL;
        }
    } else if (array->kind == 4) {
        HandleCell* cells = (HandleCell*)data_of(array);
        for (i = 0; i < array->count; ++i) {
            variant_release((struct RtVariant*)cells[i].handle);
            cells[i].handle = NULL;
        }
    } else if (array->kind == 5) {
        unsupported();
    }
}

static size_t array_bytes(const RtArray* array) {
    return sizeof(RtArray) + (size_t)array->count * (size_t)array->size;
}

/* A copy of an array (its handles are shared, arrays inside records copied). */
static RtArray* array_clone(const RtArray* source) {
    RtArray* copy;
    if (source == NULL) {
        return NULL;
    }
    copy = (RtArray*)malloc(array_bytes(source));
    if (copy == NULL) {
        raise_error(7);
        return NULL;
    }
    memcpy(copy, source, array_bytes(source));
    array_retain_elements(copy);
    return copy;
}

/* `target = source` for arrays: the target becomes a copy. */
static void array_assign(RtArray** variable, RtArray* source) {
    RtArray* copy;
    if (source == NULL) {
        array_release(*variable);
        *variable = NULL;
        return;
    }
    if (*variable == source) {
        return;
    }
    copy = (RtArray*)malloc(array_bytes(source));
    if (copy == NULL) {
        raise_error(7);
        return;
    }
    memcpy(copy, source, array_bytes(source));
    /* The target keeps being fixed-size or dynamic. */
    copy->fixed = *variable != NULL && (*variable)->fixed != 0 ? 1 : 0;
    array_retain_elements(copy);
    array_release(*variable);
    *variable = copy;
}

static void array_release(RtArray* array) {
    if (array != NULL) {
        release_elements(array);
        free(array);
    }
}

static void array_redim(RtArray** variable, int32_t flags, int32_t dims,
                        const int64_t* bounds, int32_t preserve,
                        int32_t record_size, const int32_t* record_strings) {
    int32_t kind = flags & 0xFF;
    RtArray header;
    RtArray* array;
    RtArray* old;
    int empty_allowed = (flags & 0x200) != 0;
    int64_t count = 1;
    int32_t k;
    if (dims < 1 || dims > 3) {
        raise_error(9);
        return;
    }
    if (kind > 4) {
        unsupported();
    }
    memset(&header, 0, sizeof(header));
    header.kind = kind;
    header.dims = dims;
    header.fixed = (flags >> 8) & 1;
    header.size = kind == 0 ? 4 : (kind == 3 ? record_size : 8);
    header.strings = kind == 3 ? record_strings : NULL;
    for (k = 0; k < dims; ++k) {
        int32_t lower = (int32_t)bounds[2 * k];
        int32_t upper = (int32_t)bounds[2 * k + 1];
        if (upper < lower && !(empty_allowed && upper == lower - 1)) {
            raise_error(9);
            return;
        }
        header.bounds[k].lower = lower;
        header.bounds[k].upper = upper;
        count *= (int64_t)upper - lower + 1;
        if (count > (1LL << 28)) {
            raise_error(7);
            return;
        }
    }
    header.count = (int32_t)count;
    if (preserve != 0 && *variable != NULL && (*variable)->dims == dims) {
        /* Only the last dimension of a preserved array may change. */
        for (k = 0; k + 1 < dims; ++k) {
            if ((*variable)->bounds[k].lower != header.bounds[k].lower ||
                (*variable)->bounds[k].upper != header.bounds[k].upper) {
                raise_error(9);
                return;
            }
        }
    }
    array = (RtArray*)calloc(
        1, sizeof(RtArray) + (size_t)count * (size_t)header.size);
    if (array == NULL) {
        raise_error(7);
        return;
    }
    *array = header;
    if (kind == 2 && (flags >> 16) > 0) {
        /* Elements of fixed-length Strings begin as blanks. */
        HandleCell* cells = (HandleCell*)data_of(array);
        int64_t i;
        for (i = 0; i < count; ++i) {
            cells[i].handle = string_fixed(NULL, flags >> 16);
        }
    }
    old = *variable;
    if (preserve != 0 && old != NULL && old->dims == dims &&
        old->kind == kind) {
        /* Elements that exist in both keep their values. */
        size_t size = (size_t)header.size;
        int32_t at;
        for (at = 0; at < old->count; ++at) {
            int32_t index[3] = {0, 0, 0};
            int32_t rest = at;
            int64_t target = 0;
            int inside = 1;
            for (k = dims - 1; k >= 0; --k) {
                int32_t extent =
                    old->bounds[k].upper - old->bounds[k].lower + 1;
                index[k] = old->bounds[k].lower + rest % extent;
                rest /= extent;
            }
            for (k = 0; k < dims; ++k) {
                if (index[k] < header.bounds[k].lower ||
                    index[k] > header.bounds[k].upper) {
                    inside = 0;
                    break;
                }
                target = target * ((int64_t)header.bounds[k].upper -
                                   header.bounds[k].lower + 1) +
                         (index[k] - header.bounds[k].lower);
            }
            if (inside) {
                char* from = data_of(old) + (size_t)at * size;
                memcpy(data_of(array) + (size_t)target * size, from, size);
                if (kind == 2 || kind == 4) {
                    memset(from, 0, size); /* the handle moved */
                } else if (kind == 3 && old->strings != NULL) {
                    for (k = 0; k < old->strings[0]; ++k) {
                        memset(from + (old->strings[1 + k] & IN_RECORD_MASK), 0,
                               8U);
                    }
                }
            }
        }
    }
    array_release(old);
    *variable = array;
}

/* The offset of an index inside a dimension, or -1 after raising error 9. */
static int64_t offset_in(const RtArray* array, int32_t k, int32_t index) {
    const RtBound* bound = &array->bounds[k];
    if (index < bound->lower || index > bound->upper) {
        raise_error(9);
        return -1;
    }
    return (int64_t)index - bound->lower;
}

static int64_t extent_of(const RtArray* array, int32_t k) {
    return (int64_t)array->bounds[k].upper - array->bounds[k].lower + 1;
}

static int has_dims(const RtArray* array, int32_t dims) {
    if (array == NULL || array->dims != dims) {
        raise_error(9);
        return 0;
    }
    return 1;
}

static void* element_at(RtArray* array, int64_t at) {
    return at < 0 ? NULL : data_of(array) + (size_t)at * (size_t)array->size;
}

static void* array_element1(RtArray* array, int32_t i) {
    if (!has_dims(array, 1)) {
        return NULL;
    }
    return element_at(array, offset_in(array, 0, i));
}

static void* array_element2(RtArray* array, int32_t i, int32_t j) {
    int64_t a;
    int64_t b;
    if (!has_dims(array, 2)) {
        return NULL;
    }
    a = offset_in(array, 0, i);
    b = offset_in(array, 1, j);
    return element_at(array, a < 0 || b < 0 ? -1 : a * extent_of(array, 1) + b);
}

static void* array_element3(RtArray* array, int32_t i, int32_t j, int32_t k) {
    int64_t a;
    int64_t b;
    int64_t c;
    if (!has_dims(array, 3)) {
        return NULL;
    }
    a = offset_in(array, 0, i);
    b = offset_in(array, 1, j);
    c = offset_in(array, 2, k);
    return element_at(
        array, a < 0 || b < 0 || c < 0
                   ? -1
                   : (a * extent_of(array, 1) + b) * extent_of(array, 2) + c);
}

static int32_t array_bound(const RtArray* array, int32_t dimension,
                           int32_t which) {
    if (array == NULL || dimension < 1 || dimension > array->dims) {
        raise_error(9);
        return 0;
    }
    return which == 0 ? array->bounds[dimension - 1].lower
                      : array->bounds[dimension - 1].upper;
}

static int32_t array_count(const RtArray* array) {
    return array == NULL ? 0 : array->count;
}

static void* array_flat_element(RtArray* array, int32_t index) {
    if (array == NULL || index < 0 || index >= array->count) {
        raise_error(9);
        return NULL;
    }
    return element_at(array, index);
}

static void array_erase(RtArray** variable) {
    RtArray* array = *variable;
    if (array == NULL) {
        return;
    }
    if (array->fixed != 0) {
        release_elements(array);
        memset(data_of(array), 0, (size_t)array->count * (size_t)array->size);
        return;
    }
    array_release(array);
    *variable = NULL;
}

/* ----- The error services
 * --------------------------------------------------------- */

static int32_t error_number(void) {
    return g_error_number;
}

static RtString* error_description(void) {
    return make_cstring(g_error_description);
}

static RtString* error_source(void) {
    return make_cstring(g_error_source);
}

static void error_clear(void) {
    g_error_number = 0;
    g_error_description[0] = '\0';
    g_error_source[0] = '\0';
    g_help_file[0] = '\0';
    g_help_context = 0;
}

static RtString* error_help_file(void) {
    return make_cstring(g_help_file);
}

static int32_t error_help_context(void) {
    return g_help_context;
}

static void error_raise(int32_t number, const RtString* source,
                        const RtString* description) {
    raise_error(number);
    if (length_of(description) != 0) {
        size_t n = (size_t)description->length < TEXT_MAX - 1U
                       ? (size_t)description->length
                       : TEXT_MAX - 1U;
        memcpy(g_error_description, description->data, n);
        g_error_description[n] = '\0';
    }
    if (length_of(source) != 0) {
        size_t n = (size_t)source->length < TEXT_MAX - 1U
                       ? (size_t)source->length
                       : TEXT_MAX - 1U;
        memcpy(g_error_source, source->data, n);
        g_error_source[n] = '\0';
    }
}

static void error_raise_help(int32_t number, const RtString* source,
                             const RtString* description,
                             const RtString* help_file, int32_t help_context) {
    error_raise(number, source, description);
    if (length_of(help_file) != 0) {
        size_t n = (size_t)help_file->length < TEXT_MAX - 1U
                       ? (size_t)help_file->length
                       : TEXT_MAX - 1U;
        memcpy(g_help_file, help_file->data, n);
        g_help_file[n] = '\0';
    }
    g_help_context = help_context;
}

/* A handler took over: the error is no longer pending. */
static void error_acknowledge(void) {
    if (g_pending != NULL) {
        *g_pending = 0;
    }
}

static void trace_call(const RtString* name) {
    if (name != NULL) {
        fwrite(name->data, 1U, (size_t)name->length, stderr);
    }
    fputc('\n', stderr);
}

/* ----- The entry table: the order of ir::Service
 * ----------------------------------- */

#define SERVICE_COUNT 123

static void* g_table[SERVICE_COUNT];

/* ----- Variants (ADR-0009)
 * ---------------------------------------------------------
 *
 * A handle points to an RtVariant: a count and a value of one of the types of
 * a console program (Integer, Long, Single, Double, String, Boolean, Byte,
 * Null); the null handle is Empty. The rules are those of the interpreter
 * (src/interpreter/expressions.cpp, native_engine.cpp) for these types. A
 * service that fails raises the error and returns the null handle or 0. */

enum {
    VT_EMPTY = 0,
    VT_NULL = 1,
    VT_I2 = 2,
    VT_I4 = 3,
    VT_R4 = 4,
    VT_R8 = 5,
    VT_BSTR = 8,
    VT_BOOL = 11,
    VT_UI1 = 17
};

typedef struct RtVariant {
    int32_t refs;
    int32_t type;
    union {
        int32_t l;
        float f;
        double d;
        RtString* s;
    } u;
} RtVariant;

/* The argument of an omitted Optional Variant: an Empty that IsMissing knows.
 */
static RtVariant g_missing = {1000000000, VT_EMPTY, {0}};

static int vtype(const RtVariant* handle) {
    return handle == NULL ? VT_EMPTY : handle->type;
}

static RtVariant* new_variant(int type) {
    RtVariant* handle = (RtVariant*)malloc(sizeof(RtVariant));
    if (handle == NULL) {
        die("out of memory");
    }
    memset(handle, 0, sizeof(*handle));
    handle->refs = 1;
    handle->type = type;
    return handle;
}

static RtVariant* variant_retain(RtVariant* handle) {
    if (handle != NULL && handle->refs > 0) {
        ++handle->refs;
    }
    return handle;
}

static void variant_release(RtVariant* handle) {
    if (handle != NULL && handle->refs > 0 && --handle->refs == 0) {
        if (handle->type == VT_BSTR) {
            release(handle->u.s);
        }
        free(handle);
    }
}

static void variant_addref(RtVariant* handle) {
    variant_retain(handle);
}

static void variant_assign(RtVariant** variable, RtVariant* value) {
    variant_retain(value);
    variant_release(*variable);
    *variable = value;
}

static RtVariant* variant_from_long(int32_t value) {
    RtVariant* handle = new_variant(VT_I4);
    handle->u.l = value;
    return handle;
}

static RtVariant* variant_from_integer(int32_t value) {
    RtVariant* handle = new_variant(VT_I2);
    handle->u.l = (int16_t)value;
    return handle;
}

static RtVariant* variant_from_byte(int32_t value) {
    RtVariant* handle = new_variant(VT_UI1);
    handle->u.l = value & 0xFF;
    return handle;
}

static RtVariant* variant_from_double(double value) {
    RtVariant* handle = new_variant(VT_R8);
    handle->u.d = value;
    return handle;
}

static RtVariant* variant_from_single(double value) {
    RtVariant* handle = new_variant(VT_R4);
    handle->u.f = (float)value;
    return handle;
}

static RtVariant* variant_from_string(RtString* text) {
    RtVariant* handle = new_variant(VT_BSTR);
    handle->u.s = retain(text);
    return handle;
}

static RtVariant* variant_from_boolean(int32_t value) {
    RtVariant* handle = new_variant(VT_BOOL);
    handle->u.l = value != 0 ? -1 : 0;
    return handle;
}

static RtVariant* variant_null(void) {
    return new_variant(VT_NULL);
}

static RtVariant* variant_missing(void) {
    ++g_missing.refs;
    return &g_missing;
}

static int32_t variant_is_missing(const RtVariant* handle) {
    return handle == &g_missing ? -1 : 0;
}

/* An error of the operators: the pending error, and no value. */
static RtVariant* variant_fail(int32_t number) {
    raise_error(number);
    return NULL;
}

static int is_whole_type(int type) {
    return type == VT_I2 || type == VT_I4 || type == VT_UI1;
}

static int is_number_type(int type) {
    return is_whole_type(type) || type == VT_R4 || type == VT_R8;
}

static double number_value(const RtVariant* handle) {
    switch (vtype(handle)) {
        case VT_I2:
        case VT_I4:
        case VT_UI1:
        case VT_BOOL:
            return (double)handle->u.l;
        case VT_R4:
            return (double)handle->u.f;
        case VT_R8:
            return handle->u.d;
        default:
            return 0.0;
    }
}

static int is_space_char(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int is_digit_char(char c) {
    return c >= '0' && c <= '9';
}

/* The strict numeric String form of the conversions (parse_numeric_string of
 * the interpreter): spaces around, a sign, `(5)`, `5-`, `$5`, `1D2`, `&H1F`,
 * `1,000`. Returns 1 and the value, or 0. */
static int parse_numeric(const char* text, size_t size, double* value) {
    char body[128];
    size_t first = 0;
    size_t last = size;
    size_t length;
    size_t i;
    int negative = 0;
    int rewritten = 0;
    size_t out = 0;
    char* end;
    while (first < last && is_space_char(text[first])) {
        ++first;
    }
    while (last > first && is_space_char(text[last - 1U])) {
        --last;
    }
    length = last - first;
    if (length == 0U || length >= sizeof(body) - 2U) {
        return 0;
    }
    memcpy(body, text + first, length);
    body[length] = '\0';
    {
        const char* b = body;
        size_t n = length;
        char normalized[128];
#define TRIM_SPACES()                    \
    while (n > 0U && b[0] == ' ') {      \
        ++b;                             \
        --n;                             \
    }                                    \
    while (n > 0U && b[n - 1U] == ' ') { \
        --n;                             \
    }
        if (n >= 2U && b[0] == '(' && b[n - 1U] == ')') {
            negative = 1;
            rewritten = 1;
            ++b;
            n -= 2U;
            TRIM_SPACES()
        }
        if (n >= 2U && (b[n - 1U] == '-' || b[n - 1U] == '+') &&
            is_digit_char(b[n - 2U])) {
            negative = negative != (b[n - 1U] == '-');
            rewritten = 1;
            --n;
            TRIM_SPACES()
        }
        if (n > 0U && (b[0] == '-' || b[0] == '+')) {
            negative = negative != (b[0] == '-');
            ++b;
            --n;
            TRIM_SPACES()
        }
        if (n > 0U && b[0] == '$') {
            rewritten = 1;
            ++b;
            --n;
            TRIM_SPACES()
            if (n > 0U && (b[0] == '-' || b[0] == '+')) {
                negative = negative != (b[0] == '-');
                ++b;
                --n;
                TRIM_SPACES()
            }
        }
        out = 0;
        if (n > 0U && b[0] != '&') {
            for (i = 0; i < n; ++i) {
                char c = b[i];
                if ((c == 'd' || c == 'D') && i > 0U &&
                    is_digit_char(b[i - 1U]) && i + 1U < n &&
                    (is_digit_char(b[i + 1U]) || b[i + 1U] == '+' ||
                     b[i + 1U] == '-')) {
                    rewritten = 1;
                    normalized[out++] = 'e';
                } else {
                    normalized[out++] = c;
                }
            }
        } else {
            for (i = 0; i < n; ++i) {
                normalized[out++] = b[i];
            }
        }
        normalized[out] = '\0';
        if (rewritten && out > 0U && normalized[0] != '+' &&
            normalized[0] != '-' && normalized[0] != '$' &&
            normalized[0] != '(') {
            char joined[130];
            joined[0] = negative ? '-' : '+';
            memcpy(joined + 1, normalized, out + 1U);
            return parse_numeric(joined, out + 1U, value);
        }
#undef TRIM_SPACES
    }
    if (body[0] == '+') {
        memmove(body, body + 1, length);
        --length;
    }
    if (length == 0U) {
        return 0;
    }
    if (length > 2U && body[0] == '&' &&
        (body[1] == 'h' || body[1] == 'H' || body[1] == 'o' ||
         body[1] == 'O')) {
        int hex = body[1] == 'h' || body[1] == 'H';
        size_t stop = length;
        unsigned long long magnitude = 0;
        int short_form;
        if (body[stop - 1U] == '&') {
            --stop;
        }
        for (i = 2; i < stop; ++i) {
            char c = body[i];
            int digit = -1;
            if (c >= '0' && c <= '9') {
                digit = c - '0';
            } else if (hex && c >= 'a' && c <= 'f') {
                digit = c - 'a' + 10;
            } else if (hex && c >= 'A' && c <= 'F') {
                digit = c - 'A' + 10;
            }
            if (digit < 0 || (!hex && digit > 7) || magnitude > 0xFFFFFFFFULL) {
                return 0;
            }
            magnitude = magnitude * (hex ? 16U : 8U) + (unsigned)digit;
        }
        if (magnitude > 0xFFFFFFFFULL) {
            return 0;
        }
        short_form = body[stop - 1U] != '&' && (stop - 2U) <= (hex ? 4U : 6U) &&
                     magnitude <= 0xFFFFULL && body[length - 1U] != '&';
        *value = short_form ? (double)(int16_t)magnitude
                            : (double)(int32_t)magnitude;
        return 1;
    }
    /* Thousands separators between digits of the integer part. */
    if (memchr(body, ',', length) != NULL) {
        char stripped[128];
        int in_integer = 1;
        size_t kept = 0;
        for (i = 0; i < length; ++i) {
            char c = body[i];
            if (c == '.' || c == 'e' || c == 'E') {
                in_integer = 0;
            }
            if (c == ',' && in_integer && i > 0U && i + 1U < length &&
                is_digit_char(body[i - 1U]) && is_digit_char(body[i + 1U])) {
                continue;
            }
            stripped[kept++] = c;
        }
        stripped[kept] = '\0';
        if (memchr(stripped, ',', kept) != NULL) {
            return 0;
        }
        return parse_numeric(stripped, kept, value);
    }
    /* digits [. digits] [e [sign] digits], at least one digit */
    {
        size_t at = 0;
        int digits = 0;
        if (body[at] == '-') {
            ++at;
        }
        while (is_digit_char(body[at])) {
            ++at;
            ++digits;
        }
        if (body[at] == '.') {
            ++at;
            while (is_digit_char(body[at])) {
                ++at;
                ++digits;
            }
        }
        if (digits == 0) {
            return 0;
        }
        if (body[at] == 'e' || body[at] == 'E') {
            size_t exponent = at + 1U;
            int exponent_digits = 0;
            if (body[exponent] == '+' || body[exponent] == '-') {
                ++exponent;
            }
            while (is_digit_char(body[exponent])) {
                ++exponent;
                ++exponent_digits;
            }
            if (exponent_digits == 0) {
                return 0;
            }
            at = exponent;
        }
        if (at != length) {
            return 0;
        }
    }
    *value = strtod(body, &end);
    if (*end != '\0' || !(*value - *value == 0.0)) {
        return 0;
    }
    return 1;
}

/* The text of a value as `render` writes it (no sign space). */
static RtString* variant_render(const RtVariant* handle) {
    char buffer[64];
    switch (vtype(handle)) {
        case VT_I2:
        case VT_I4:
        case VT_UI1:
            sprintf(buffer, "%ld", (long)handle->u.l);
            return make_cstring(buffer);
        case VT_R4:
            format_float(buffer, sizeof(buffer), (double)handle->u.f, 7);
            return make_cstring(buffer);
        case VT_R8:
            format_float(buffer, sizeof(buffer), handle->u.d, 15);
            return make_cstring(buffer);
        case VT_BOOL:
            return make_cstring(handle->u.l != 0 ? "True" : "False");
        case VT_BSTR:
            return retain(handle->u.s);
        default:
            return NULL;
    }
}

/* ----- Coercions */

/* CLng of a Double: rounded to even; 6 when it does not fit. */
static int round_to_long(double value, int32_t* into) {
    double rounded = nearbyint(value);
    if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
        raise_error(6);
        return 0;
    }
    *into = (int32_t)rounded;
    return 1;
}

/* coerce_long: Null is 94, a String that is not a number is 13. */
static int coerce_long(const RtVariant* handle, int32_t* into,
                       int32_t null_error) {
    double parsed;
    switch (vtype(handle)) {
        case VT_NULL:
            raise_error(null_error);
            return 0;
        case VT_EMPTY:
            *into = 0;
            return 1;
        case VT_I2:
        case VT_I4:
        case VT_UI1:
        case VT_BOOL:
            *into = handle->u.l;
            return 1;
        case VT_R4:
            return round_to_long((double)handle->u.f, into);
        case VT_R8:
            return round_to_long(handle->u.d, into);
        case VT_BSTR:
            if (parse_numeric(handle->u.s == NULL ? "" : handle->u.s->data,
                              (size_t)length_of(handle->u.s), &parsed) &&
                fabs(parsed) < 2147483647.5) {
                *into = (int32_t)nearbyint(parsed);
                return 1;
            }
            raise_error(13);
            return 0;
        default:
            raise_error(13);
            return 0;
    }
}

/* number_of: Empty is 0, a Boolean -1 or 0, a String must be numeric. */
static int number_of(const RtVariant* handle, double* into) {
    switch (vtype(handle)) {
        case VT_EMPTY:
            *into = 0.0;
            return 1;
        case VT_BOOL:
        case VT_I2:
        case VT_I4:
        case VT_UI1:
        case VT_R4:
        case VT_R8:
            *into = number_value(handle);
            return 1;
        case VT_BSTR:
            return parse_numeric(handle->u.s == NULL ? "" : handle->u.s->data,
                                 (size_t)length_of(handle->u.s), into);
        default:
            return 0;
    }
}

static int32_t variant_to_long(const RtVariant* handle) {
    int32_t value = 0;
    if (!coerce_long(handle, &value, 94)) {
        return 0;
    }
    return value;
}

static double variant_to_double(const RtVariant* handle) {
    double value = 0.0;
    if (vtype(handle) == VT_NULL) {
        raise_error(94);
        return 0.0;
    }
    if (!number_of(handle, &value)) {
        raise_error(13);
        return 0.0;
    }
    return value;
}

static RtString* variant_to_string(const RtVariant* handle) {
    if (vtype(handle) == VT_NULL) {
        raise_error(94);
        return NULL;
    }
    return variant_render(handle);
}

static int32_t variant_to_boolean(const RtVariant* handle) {
    double parsed;
    switch (vtype(handle)) {
        case VT_NULL:
        case VT_EMPTY:
            return 0;
        case VT_BOOL:
            return handle->u.l != 0 ? -1 : 0;
        case VT_I2:
        case VT_I4:
        case VT_UI1:
        case VT_R4:
        case VT_R8:
            return number_value(handle) != 0.0 ? -1 : 0;
        case VT_BSTR: {
            const RtString* text = handle->u.s;
            int32_t n = length_of(text);
            if (n == 4 && (text->data[0] | 32) == 't' &&
                (text->data[1] | 32) == 'r' && (text->data[2] | 32) == 'u' &&
                (text->data[3] | 32) == 'e') {
                return -1;
            }
            if (n == 5 && (text->data[0] | 32) == 'f' &&
                (text->data[1] | 32) == 'a' && (text->data[2] | 32) == 'l' &&
                (text->data[3] | 32) == 's' && (text->data[4] | 32) == 'e') {
                return 0;
            }
            if (parse_numeric(text == NULL ? "" : text->data, (size_t)n,
                              &parsed)) {
                return parsed != 0.0 ? -1 : 0;
            }
            raise_error(13);
            return 0;
        }
        default:
            raise_error(13);
            return 0;
    }
}

/* ----- Operators */

static RtVariant* variant_double_result(double value) {
    if (!(value - value == 0.0)) {
        return variant_fail(6);
    }
    return variant_from_double(value);
}

static RtVariant* variant_whole_result(int type, int64_t value) {
    RtVariant* handle = new_variant(type);
    handle->u.l = (int32_t)value;
    return handle;
}


/* ----- Comparisons */

static RtVariant* compare_result(int ordering, int op) {
    int answer;
    switch (op) {
        case 8:
            answer = ordering == 0;
            break;
        case 9:
            answer = ordering != 0;
            break;
        case 10:
            answer = ordering < 0;
            break;
        case 11:
            answer = ordering > 0;
            break;
        case 12:
            answer = ordering <= 0;
            break;
        default:
            answer = ordering >= 0;
            break;
    }
    return variant_from_boolean(answer);
}

/* ----- Comparisons as Visual Basic 6 makes them
 *
 * The same rules as Interpreter::operator_order (src/interpreter/
 * select_case.cpp), which were measured under Visual Basic 6, for the types
 * this runtime has: Null gives Null; Empty is 0 beside a number and "" beside a
 * String; two typed operands convert a String to the type of the other
 * (error 13 when it is not a number); a typed String beside a Variant compares
 * as text; a Variant String beside a typed number or Boolean is converted when
 * it can be and is the greater when it cannot; two Variants put every number
 * before every String. A Boolean is 255 beside a Byte. */

static int order_doubles(double a, double b) {
    return a < b ? -1 : (a > b ? 1 : 0);
}

static int order_numbers_vb(const RtVariant* a, const RtVariant* b) {
    int at = vtype(a);
    int bt = vtype(b);
    if (at == VT_UI1 && bt == VT_BOOL) {
        return order_doubles(number_value(a), b->u.l != 0 ? 255.0 : 0.0);
    }
    if (at == VT_BOOL && bt == VT_UI1) {
        return order_doubles(a->u.l != 0 ? 255.0 : 0.0, number_value(b));
    }
    if ((at == VT_R4 || bt == VT_R4) && at != VT_BOOL && bt != VT_BOOL) {
        return order_doubles((double)(float)number_value(a),
                             (double)(float)number_value(b));
    }
    return order_doubles(number_value(a), number_value(b));
}

/* A String as a Boolean: True, False or a number; 0 when it is none. */
static int text_to_boolean(const RtString* text, int* answer) {
    char body[16];
    size_t first = 0;
    size_t last = (size_t)length_of(text);
    size_t i;
    double parsed;
    while (first < last && text->data[first] == ' ') {
        ++first;
    }
    while (last > first && text->data[last - 1U] == ' ') {
        --last;
    }
    if (last - first == 4U || last - first == 5U) {
        for (i = 0; i < last - first; ++i) {
            char c = text->data[first + i];
            body[i] = (char)(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
        }
        body[last - first] = '\0';
        if (strcmp(body, "true") == 0) {
            *answer = -1;
            return 1;
        }
        if (strcmp(body, "false") == 0) {
            *answer = 0;
            return 1;
        }
    }
    if (!parse_numeric(text == NULL ? "" : text->data, (size_t)length_of(text),
                       &parsed)) {
        return 0;
    }
    *answer = parsed != 0.0 ? -1 : 0;
    return 1;
}

static RtVariant* variant_compare_vb(const RtVariant* a, const RtVariant* b,
                                     int op, int a_variant, int b_variant) {
    int at = vtype(a);
    int bt = vtype(b);
    int a_typed = !a_variant && at != VT_EMPTY && at != VT_NULL;
    int b_typed = !b_variant && bt != VT_EMPTY && bt != VT_NULL;
    int ordering;
    if (at == VT_NULL || bt == VT_NULL) {
        return variant_null();
    }
    if (at == VT_EMPTY && bt == VT_EMPTY) {
        return compare_result(0, op);
    }
    if (at == VT_EMPTY || bt == VT_EMPTY) {
        int first = at == VT_EMPTY;
        const RtVariant* other = first ? b : a;
        ordering = vtype(other) == VT_BSTR
                       ? string_compare(NULL, other->u.s)
                       : order_doubles(0.0, number_value(other));
        return compare_result(first ? ordering : -ordering, op);
    }
    if (a_typed && b_typed) {
        if (at == VT_BSTR && bt == VT_BSTR) {
            return compare_result(string_compare(a->u.s, b->u.s), op);
        }
        if (at == VT_BSTR || bt == VT_BSTR) {
            int first = at == VT_BSTR;
            const RtVariant* text = first ? a : b;
            const RtVariant* other = first ? b : a;
            if (vtype(other) == VT_BOOL) {
                int truth;
                if (!text_to_boolean(text->u.s, &truth)) {
                    return variant_fail(13);
                }
                ordering = order_doubles((double)truth, number_value(other));
            } else {
                double parsed;
                RtVariant number;
                if (!parse_numeric(text->u.s == NULL ? "" : text->u.s->data,
                                   (size_t)length_of(text->u.s), &parsed)) {
                    return variant_fail(13);
                }
                memset(&number, 0, sizeof(number));
                number.refs = 1000000000;
                number.type = VT_R8;
                number.u.d = parsed;
                ordering = order_numbers_vb(&number, other);
            }
            return compare_result(first ? ordering : -ordering, op);
        }
        return compare_result(order_numbers_vb(a, b), op);
    }
    /* A Variant is involved. */
    if (at == VT_BSTR && bt == VT_BSTR) {
        return compare_result(string_compare(a->u.s, b->u.s), op);
    }
    if (at == VT_BSTR || bt == VT_BSTR) {
        int first = at == VT_BSTR;
        const RtVariant* text = first ? a : b;
        const RtVariant* other = first ? b : a;
        int text_typed = first ? a_typed : b_typed;
        int other_typed = first ? b_typed : a_typed;
        int sign = first ? 1 : -1;
        if (text_typed) {
            RtString* rendered = variant_render(other);
            ordering = string_compare(text->u.s, rendered);
            release(rendered);
            return compare_result(sign * ordering, op);
        }
        if (other_typed) {
            double parsed;
            RtVariant number;
            if (vtype(other) == VT_BOOL) {
                int truth;
                if (!text_to_boolean(text->u.s, &truth)) {
                    return compare_result(sign, op);
                }
                return compare_result(
                    sign * order_doubles((double)truth, number_value(other)),
                    op);
            }
            if (!parse_numeric(text->u.s == NULL ? "" : text->u.s->data,
                               (size_t)length_of(text->u.s), &parsed)) {
                return compare_result(sign, op);
            }
            memset(&number, 0, sizeof(number));
            number.refs = 1000000000;
            number.type = VT_R8;
            number.u.d = parsed;
            return compare_result(sign * order_numbers_vb(&number, other), op);
        }
        return compare_result(sign, op); /* every String is the greater */
    }
    return compare_result(order_numbers_vb(a, b), op);
}

/* ----- Concatenation and power */

static RtVariant* variant_concatenate(const RtVariant* left,
                                      const RtVariant* right) {
    RtString* a = NULL;
    RtString* b = NULL;
    RtVariant* handle;
    int left_null = vtype(left) == VT_NULL;
    int right_null = vtype(right) == VT_NULL;
    if (left_null && right_null) {
        return variant_null();
    }
    if (!left_null) {
        a = variant_render(left);
    }
    if (!right_null) {
        b = variant_render(right);
    }
    handle = new_variant(VT_BSTR);
    handle->u.s = string_concat(a, b);
    release(a);
    release(b);
    return handle;
}

/* ----- The arithmetic and logical operators of Visual Basic 6
 *
 * The rules were measured under Visual Basic 6 (src/interpreter/arithmetic.cpp
 * has them in full); this is the same for the types this runtime has. */

enum { NUM_NONE = -1, NUM_BYTE, NUM_INT, NUM_LONG, NUM_SINGLE, NUM_DOUBLE };

/* An operand as a number: its type (NUM_NONE for Empty, which adopts the
 * other's), its value, and whether a String was read. 0 when a String is not a
 * number. */
static int ar_operand(const RtVariant* handle, int* type, double* value) {
    switch (vtype(handle)) {
        case VT_EMPTY:
            *type = NUM_NONE;
            *value = 0.0;
            return 1;
        case VT_BOOL:
            *type = NUM_INT;
            *value = handle->u.l != 0 ? -1.0 : 0.0;
            return 1;
        case VT_UI1:
            *type = NUM_BYTE;
            *value = (double)handle->u.l;
            return 1;
        case VT_I2:
            *type = NUM_INT;
            *value = (double)handle->u.l;
            return 1;
        case VT_I4:
            *type = NUM_LONG;
            *value = (double)handle->u.l;
            return 1;
        case VT_R4:
            *type = NUM_SINGLE;
            *value = (double)handle->u.f;
            return 1;
        case VT_R8:
            *type = NUM_DOUBLE;
            *value = handle->u.d;
            return 1;
        case VT_BSTR:
            *type = NUM_DOUBLE;
            return parse_numeric(handle->u.s == NULL ? "" : handle->u.s->data,
                                 (size_t)length_of(handle->u.s), value);
        default:
            return 0;
    }
}

static int ar_numeric_text(const RtVariant* handle) {
    double value;
    return vtype(handle) == VT_BSTR &&
           parse_numeric(handle->u.s == NULL ? "" : handle->u.s->data,
                         (size_t)length_of(handle->u.s), &value);
}

static int ar_is_true_text(const RtVariant* handle) {
    return vtype(handle) == VT_BSTR && handle->u.s != NULL &&
           handle->u.s->length == 4 &&
           memcmp(handle->u.s->data, "True", 4) == 0;
}

/* The Null case of an operator that reads its operands as numbers: a String
 * that is not a number is still error 13. Returns 0 when neither is Null, 1
 * with `*result` set otherwise. */
static int ar_null_case(const RtVariant* a, const RtVariant* b, int allow_true,
                        RtVariant** result) {
    const RtVariant* other;
    if (vtype(a) != VT_NULL && vtype(b) != VT_NULL) {
        return 0;
    }
    other = vtype(a) == VT_NULL ? b : a;
    if (vtype(other) == VT_BSTR && !ar_numeric_text(other) &&
        !(allow_true && ar_is_true_text(other))) {
        *result = variant_fail(13);
    } else {
        *result = variant_null();
    }
    return 1;
}

static int ar_fits(int type, int64_t value) {
    switch (type) {
        case NUM_BYTE:
            return value >= 0 && value <= 255;
        case NUM_INT:
            return value >= -32768 && value <= 32767;
        default:
            return value >= INT32_MIN && value <= INT32_MAX;
    }
}

static RtVariant* ar_whole(int type, int64_t value) {
    return variant_whole_result(
        type == NUM_BYTE ? VT_UI1 : (type == NUM_INT ? VT_I2 : VT_I4), value);
}

static RtVariant* ar_add(int op, const RtVariant* a, const RtVariant* b,
                         int typed) {
    int at = vtype(a);
    int bt = vtype(b);
    int ta;
    int tb;
    int type;
    double x;
    double y;
    RtVariant* result;
    if (op == '+' && (at == VT_NULL || bt == VT_NULL)) {
        return variant_null(); /* a String joins under +, so it never fails */
    }
    if (ar_null_case(a, b, 0, &result)) {
        return result;
    }
    if (op == '+' && at == VT_BSTR && bt == VT_BSTR) {
        result = new_variant(VT_BSTR);
        result->u.s = string_concat(a->u.s, b->u.s);
        return result;
    }
    if (op == '+' && ((at == VT_BSTR && bt == VT_EMPTY) ||
                      (at == VT_EMPTY && bt == VT_BSTR))) {
        return variant_from_string((at == VT_BSTR ? a : b)->u.s);
    }
    if (!ar_operand(a, &ta, &x) || !ar_operand(b, &tb, &y)) {
        return variant_fail(13);
    }
    if (ta == NUM_NONE && tb == NUM_NONE) {
        ta = tb = NUM_INT;
    } else if (ta == NUM_NONE) {
        ta = tb;
    } else if (tb == NUM_NONE) {
        tb = ta;
    }
    if (ta == NUM_DOUBLE || tb == NUM_DOUBLE) {
        type = NUM_DOUBLE;
    } else if (ta == NUM_SINGLE || tb == NUM_SINGLE) {
        type = ta == NUM_LONG || tb == NUM_LONG ? NUM_DOUBLE : NUM_SINGLE;
    } else if (ta == NUM_LONG || tb == NUM_LONG) {
        type = NUM_LONG;
    } else if (ta == NUM_BYTE && tb == NUM_BYTE) {
        type = NUM_BYTE;
    } else {
        type = NUM_INT;
    }
    if (type <= NUM_LONG) {
        int64_t l = (int64_t)x;
        int64_t r = (int64_t)y;
        int64_t z = op == '+' ? l + r : (op == '-' ? l - r : l * r);
        while (!ar_fits(type, z)) {
            if (typed) {
                return variant_fail(6);
            }
            if (type == NUM_LONG) {
                return variant_from_double((double)z);
            }
            type = type == NUM_BYTE ? NUM_INT : NUM_LONG;
        }
        return ar_whole(type, z);
    }
    if (type == NUM_SINGLE) {
        float l = (float)x;
        float r = (float)y;
        float z = op == '+' ? l + r : (op == '-' ? l - r : l * r);
        if (!((double)z - (double)z == 0.0)) {
            return variant_fail(6);
        }
        return variant_from_single((double)z);
    }
    return variant_double_result(op == '+' ? x + y
                                           : (op == '-' ? x - y : x * y));
}

static RtVariant* ar_divide(const RtVariant* a, const RtVariant* b, int typed) {
    int ta;
    int tb;
    double x;
    double y;
    RtVariant* result;
    int small_a;
    int small_b;
    if (ar_null_case(a, b, 0, &result)) {
        return result;
    }
    if (!ar_operand(a, &ta, &x) || !ar_operand(b, &tb, &y)) {
        return variant_fail(13);
    }
    if (y == 0.0) {
        return variant_fail(x != 0.0 && typed ? 11 : 6);
    }
    small_a = vtype(a) == VT_R4 || vtype(a) == VT_I2 || vtype(a) == VT_UI1 ||
              vtype(a) == VT_BOOL || vtype(a) == VT_EMPTY;
    small_b = vtype(b) == VT_R4 || vtype(b) == VT_I2 || vtype(b) == VT_UI1 ||
              vtype(b) == VT_BOOL || vtype(b) == VT_EMPTY;
    if (small_a && small_b && (vtype(a) == VT_R4 || vtype(b) == VT_R4)) {
        float z = (float)x / (float)y;
        if (!((double)z - (double)z == 0.0)) {
            return variant_fail(6);
        }
        return variant_from_single((double)z);
    }
    return variant_double_result(x / y);
}

/* An operand read as a whole number. */
typedef struct ArWhole {
    int type; /* NUM_ type; NUM_DOUBLE for what was rounded */
    int boolean;
    int64_t value;
    int error;
} ArWhole;

static int64_t ar_round_long(double value, int* error) {
    double rounded = nearbyint(value);
    if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
        *error = 6;
        return 0;
    }
    return (int64_t)rounded;
}

static ArWhole ar_whole_of(const RtVariant* handle, int other_typed) {
    ArWhole whole;
    double number;
    whole.type = NUM_DOUBLE;
    whole.boolean = 0;
    whole.value = 0;
    whole.error = 0;
    switch (vtype(handle)) {
        case VT_EMPTY:
            whole.type = NUM_INT;
            return whole;
        case VT_BSTR:
            if (ar_numeric_text(handle)) {
                ar_operand(handle, &whole.type, &number);
                whole.type = NUM_DOUBLE;
                whole.value = ar_round_long(number, &whole.error);
                return whole;
            }
            if (ar_is_true_text(handle) && !other_typed) {
                whole.type = NUM_INT;
                whole.boolean = 1;
                whole.value = -1;
                return whole;
            }
            whole.error = 13;
            return whole;
        case VT_BOOL:
            whole.type = NUM_INT;
            whole.boolean = 1;
            whole.value = handle->u.l != 0 ? -1 : 0;
            return whole;
        case VT_UI1:
            whole.type = NUM_BYTE;
            whole.value = handle->u.l;
            return whole;
        case VT_I2:
            whole.type = NUM_INT;
            whole.value = handle->u.l;
            return whole;
        case VT_I4:
            whole.type = NUM_LONG;
            whole.value = handle->u.l;
            return whole;
        default:
            ar_operand(handle, &whole.type, &number);
            whole.type = NUM_DOUBLE;
            whole.value = ar_round_long(number, &whole.error);
            return whole;
    }
}

static int ar_small_whole(const ArWhole* whole) {
    return whole->boolean || whole->type == NUM_INT || whole->type == NUM_BYTE;
}

static RtVariant* ar_whole_division(char op, const RtVariant* a,
                                    const RtVariant* b, int a_typed,
                                    int b_typed) {
    ArWhole x;
    ArWhole y;
    int64_t z;
    int type;
    RtVariant* result;
    if (ar_null_case(a, b, 1, &result)) {
        return result;
    }
    if (vtype(a) == VT_BOOL && a_typed && vtype(b) == VT_BSTR && b_typed &&
        (ar_numeric_text(b) || ar_is_true_text(b))) {
        return variant_from_boolean(op == '\\' && a->u.l != 0);
    }
    x = ar_whole_of(a, b_typed);
    if (x.error != 0) {
        return variant_fail(x.error);
    }
    y = ar_whole_of(b, a_typed);
    if (y.error != 0) {
        return variant_fail(y.error);
    }
    if (y.value == 0) {
        return variant_fail(11);
    }
    if (x.value == INT32_MIN && y.value == -1) {
        return variant_fail(6);
    }
    z = op == '\\' ? x.value / y.value : x.value % y.value;
    type = NUM_LONG;
    if (ar_small_whole(&x) && ar_small_whole(&y)) {
        type =
            x.type == NUM_BYTE && y.type == NUM_BYTE && !x.boolean && !y.boolean
                ? NUM_BYTE
                : NUM_INT;
    }
    if (!ar_fits(type, z)) {
        return variant_fail(6);
    }
    return ar_whole(type, z);
}

static RtVariant* ar_power(const RtVariant* a, const RtVariant* b) {
    int ta;
    int tb;
    double x;
    double y;
    RtVariant* result;
    if (ar_null_case(a, b, 0, &result)) {
        return result;
    }
    if (!ar_operand(a, &ta, &x) || !ar_operand(b, &tb, &y)) {
        return variant_fail(13);
    }
    if ((x == 0.0 && y < 0.0) || (x < 0.0 && y != (double)(int64_t)y)) {
        return variant_fail(5);
    }
    return variant_double_result(accurate_pow(x, y));
}

/* A value of the type of a whole operand. */
static RtVariant* ar_of_type(const ArWhole* whole, int64_t value) {
    if (whole->boolean) {
        return variant_from_boolean(value != 0);
    }
    return ar_whole(whole->type == NUM_DOUBLE ? NUM_LONG : whole->type,
                    whole->type == NUM_BYTE ? (value & 0xFF) : value);
}

static RtVariant* ar_logical(int code, const RtVariant* a, const RtVariant* b,
                             int a_typed, int b_typed) {
    ArWhole x;
    ArWhole y;
    int64_t r;
    x.value = 0;
    y.value = 0;
    x.type = y.type = NUM_LONG;
    x.boolean = y.boolean = 0;
    x.error = y.error = 0;
    if (vtype(a) != VT_NULL) {
        x = ar_whole_of(a, b_typed);
        if (x.error != 0) {
            return variant_fail(x.error);
        }
    }
    if (vtype(b) != VT_NULL) {
        y = ar_whole_of(b, a_typed);
        if (y.error != 0) {
            return variant_fail(y.error);
        }
    }
    if (vtype(a) == VT_NULL && vtype(b) == VT_NULL) {
        return variant_null();
    }
    if (vtype(a) == VT_NULL || vtype(b) == VT_NULL) {
        int null_left = vtype(a) == VT_NULL;
        const ArWhole* w = null_left ? &y : &x;
        int64_t v = w->value;
        switch (code) {
            case 14:
                return v == 0 ? ar_of_type(w, 0) : variant_null();
            case 15:
                return v != 0 ? ar_of_type(w, v) : variant_null();
            case 18: {
                int64_t n;
                if (null_left) {
                    return v != 0 ? ar_of_type(w, v) : variant_null();
                }
                n = ~v;
                if (w->boolean) {
                    n = v != 0 ? 0 : -1;
                } else if (w->type == NUM_BYTE) {
                    n &= 0xFF;
                }
                return n != 0 ? ar_of_type(w, n) : variant_null();
            }
            default:
                return variant_null();
        }
    }
    switch (code) {
        case 14:
            r = x.value & y.value;
            break;
        case 15:
            r = x.value | y.value;
            break;
        case 16:
            r = x.value ^ y.value;
            break;
        case 17:
            r = ~(x.value ^ y.value);
            break;
        default:
            r = ~x.value | y.value;
            break;
    }
    if (x.boolean && y.boolean) {
        return variant_from_boolean(r != 0);
    }
    if (x.type == NUM_BYTE && y.type == NUM_BYTE && !x.boolean && !y.boolean) {
        return ar_whole(NUM_BYTE, r & 0xFF);
    }
    if (ar_small_whole(&x) && ar_small_whole(&y)) {
        return ar_whole(NUM_INT, (int16_t)r);
    }
    return ar_whole(NUM_LONG, (int32_t)r);
}

static int ar_typed(const RtVariant* handle, int variant) {
    return !variant && vtype(handle) != VT_EMPTY && vtype(handle) != VT_NULL;
}

static RtVariant* variant_binary(int32_t op, const RtVariant* a,
                                 const RtVariant* b) {
    int left_variant = (op & 0x100) != 0;
    int right_variant = (op & 0x200) != 0;
    int code = op & 0xFF;
    int a_typed = ar_typed(a, left_variant);
    int b_typed = ar_typed(b, right_variant);
    switch (code) {
        case 0:
            return ar_add('+', a, b, a_typed && b_typed);
        case 1:
            return ar_add('-', a, b, a_typed && b_typed);
        case 2:
            return ar_add('*', a, b, a_typed && b_typed);
        case 3:
            return ar_divide(a, b, a_typed && b_typed);
        case 4:
            return ar_whole_division('\\', a, b, a_typed, b_typed);
        case 5:
            return ar_whole_division('%', a, b, a_typed, b_typed);
        case 6:
            return variant_concatenate(a, b);
        case 7:
            return ar_power(a, b);
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            return variant_compare_vb(a, b, code, left_variant, right_variant);
        default:
            return ar_logical(code, a, b, a_typed, b_typed);
    }
}

static RtVariant* variant_negate(const RtVariant* handle) {
    double number;
    int type;
    switch (vtype(handle)) {
        case VT_NULL:
            return variant_null();
        case VT_EMPTY:
            return variant_from_integer(0);
        case VT_BOOL:
            return ar_whole(NUM_INT, handle->u.l != 0 ? 1 : 0);
        case VT_UI1:
            return ar_whole(NUM_INT, -(int64_t)handle->u.l);
        case VT_I2:
            if (handle->u.l == -32768) {
                return variant_from_long(32768);
            }
            return ar_whole(NUM_INT, -(int64_t)handle->u.l);
        case VT_I4:
            if (handle->u.l == INT32_MIN) {
                return variant_from_double(2147483648.0);
            }
            return ar_whole(NUM_LONG, -(int64_t)handle->u.l);
        case VT_R4:
            return variant_from_single(-(double)handle->u.f);
        case VT_R8:
            return variant_from_double(-handle->u.d);
        case VT_BSTR:
            if (!ar_operand(handle, &type, &number)) {
                return variant_fail(13);
            }
            return variant_from_double(-number);
        default:
            return variant_fail(13);
    }
}

static RtVariant* variant_not(const RtVariant* handle) {
    ArWhole whole;
    RtVariant* result;
    switch (vtype(handle)) {
        case VT_NULL:
            return variant_null();
        case VT_EMPTY:
            return ar_whole(NUM_INT, -1);
        case VT_BOOL:
            return variant_from_boolean(handle->u.l == 0);
        case VT_UI1:
            return ar_whole(NUM_BYTE, (~(int64_t)handle->u.l) & 0xFF);
        case VT_I2:
            return ar_whole(NUM_INT, ~(int64_t)handle->u.l);
        case VT_I4:
            return ar_whole(NUM_LONG, ~(int64_t)handle->u.l);
        default:
            whole = ar_whole_of(handle, 1);
            if (whole.error != 0) {
                return variant_fail(whole.error);
            }
            result = ar_whole(NUM_LONG, ~whole.value);
            return result;
    }
}

/* ----- Printing and type information */

static void variant_print(const RtVariant* handle) {
    int type = vtype(handle);
    RtString* text;
    if (type == VT_NULL) {
        put_cstring("Null");
        return;
    }
    text = variant_render(handle);
    if (is_number_type(type)) {
        if (length_of(text) == 0 || text->data[0] != '-') {
            put(" ", 1U);
        }
        put(text == NULL ? "" : text->data, (size_t)length_of(text));
        put(" ", 1U);
    } else if (text != NULL) {
        put(text->data, (size_t)text->length);
    }
    release(text);
}

static RtString* variant_type_name(const RtVariant* handle) {
    switch (vtype(handle)) {
        case VT_NULL:
            return make_cstring("Null");
        case VT_I2:
            return make_cstring("Integer");
        case VT_I4:
            return make_cstring("Long");
        case VT_R4:
            return make_cstring("Single");
        case VT_R8:
            return make_cstring("Double");
        case VT_BSTR:
            return make_cstring("String");
        case VT_BOOL:
            return make_cstring("Boolean");
        case VT_UI1:
            return make_cstring("Byte");
        default:
            return make_cstring("Empty");
    }
}

static int32_t variant_var_type(const RtVariant* handle) {
    return vtype(handle);
}

static int32_t variant_is_numeric(const RtVariant* handle) {
    double parsed;
    int type = vtype(handle);
    if (type == VT_BSTR) {
        return parse_numeric(handle->u.s == NULL ? "" : handle->u.s->data,
                             (size_t)length_of(handle->u.s), &parsed)
                   ? -1
                   : 0;
    }
    return is_number_type(type) || type == VT_BOOL ? -1 : 0;
}

/* ----- Library functions on Variants (service 77)
 * ---------------------------------------
 *
 * (name, count, block): the function of the VBA library by name (lower case,
 * without a `$`), its arguments the handles of eight-byte cells read upward
 * from `block`. The names the runtime knows are listed in
 * src/compiler/small_runtime.hpp; the results are those of the interpreter. */

static RtVariant* argument_at(const char* block, int32_t index) {
    RtVariant* handle;
    memcpy(&handle, block + 8 * (size_t)index, sizeof(handle));
    return handle;
}

static RtVariant* variant_from_text(RtString* text) {
    RtVariant* handle = new_variant(VT_BSTR);
    handle->u.s = text; /* the new string's own count */
    return handle;
}

/* The operand of Abs, Int and Fix: its value, and the type of the result. */
static int math_operand(const RtVariant* handle, double* value, int* type) {
    switch (vtype(handle)) {
        case VT_EMPTY:
            *value = 0.0;
            *type = VT_I2;
            return 1;
        case VT_BOOL:
            *value = handle->u.l != 0 ? -1.0 : 0.0;
            *type = VT_I2;
            return 1;
        case VT_I2:
            *value = (double)handle->u.l;
            *type = VT_I2;
            return 1;
        case VT_I4:
        case VT_UI1:
            *value = (double)handle->u.l;
            *type = VT_I4;
            return 1;
        case VT_R4:
            *value = (double)handle->u.f;
            *type = VT_R4;
            return 1;
        case VT_R8:
            *value = handle->u.d;
            *type = VT_R8;
            return 1;
        case VT_BSTR:
            *type = VT_R8;
            if (!parse_numeric(handle->u.s == NULL ? "" : handle->u.s->data,
                               (size_t)length_of(handle->u.s), value)) {
                raise_error(13);
                return 0;
            }
            return 1;
        default:
            raise_error(13);
            return 0;
    }
}

static RtVariant* number_of_type(double value, int type) {
    RtVariant* handle;
    switch (type) {
        case VT_I2:
            return variant_whole_result(VT_I2, (int64_t)value);
        case VT_I4:
            return variant_whole_result(VT_I4, (int64_t)value);
        case VT_R4:
            handle = new_variant(VT_R4);
            handle->u.f = (float)value;
            return handle;
        default:
            return variant_from_double(value);
    }
}

/* Abs, Int, Fix: which = 0, 1, 2. */
static RtVariant* library_round(const RtVariant* argument, int which) {
    double value;
    int type;
    if (vtype(argument) == VT_NULL) {
        return variant_null();
    }
    if (!math_operand(argument, &value, &type)) {
        return NULL;
    }
    if (which == 0) {
        if (type == VT_I2 && value == -32768.0) {
            return variant_fail(6);
        }
        value = fabs(value);
    } else if (type == VT_R4 || type == VT_R8) {
        value = which == 1 ? floor(value) : trunc(value);
    }
    return number_of_type(value, type);
}

static RtVariant* library_sgn(const RtVariant* argument) {
    double value;
    int type;
    if (vtype(argument) == VT_NULL) {
        return variant_null();
    }
    if (!math_operand(argument, &value, &type)) {
        return NULL;
    }
    return variant_from_long(value > 0.0 ? 1 : (value < 0.0 ? -1 : 0));
}

static RtVariant* library_sqr(const RtVariant* argument) {
    double value;
    if (vtype(argument) == VT_NULL) {
        return variant_fail(94);
    }
    if (!number_of(argument, &value)) {
        return variant_fail(13);
    }
    if (value < 0.0) {
        return variant_fail(5);
    }
    return variant_from_double(accurate_sqrt(value));
}

/* CInt, CLng, CByte: which = 0, 1, 2. */
static RtVariant* library_convert_whole(const RtVariant* argument, int which) {
    double value;
    double rounded;
    if (vtype(argument) == VT_NULL) {
        return variant_fail(94);
    }
    if (which == 2 && vtype(argument) == VT_BOOL) {
        return variant_whole_result(VT_UI1, argument->u.l != 0 ? 255 : 0);
    }
    if (!number_of(argument, &value)) {
        return variant_fail(13);
    }
    rounded = nearbyint(value);
    if (which == 0) {
        if (!(rounded >= -32768.0 && rounded <= 32767.0)) {
            return variant_fail(6);
        }
        return variant_whole_result(VT_I2, (int64_t)rounded);
    }
    if (which == 1) {
        if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
            return variant_fail(6);
        }
        return variant_whole_result(VT_I4, (int64_t)rounded);
    }
    if (!(rounded >= 0.0 && rounded <= 255.0)) {
        return variant_fail(6);
    }
    return variant_whole_result(VT_UI1, (int64_t)rounded);
}

static RtVariant* library_cdbl(const RtVariant* argument, int single) {
    double value;
    RtVariant* handle;
    if (vtype(argument) == VT_NULL) {
        return variant_fail(94);
    }
    if (!number_of(argument, &value)) {
        return variant_fail(13);
    }
    if (!single) {
        return variant_from_double(value);
    }
    if (fabs(value) > 3.4028234663852886e38) {
        return variant_fail(6);
    }
    handle = new_variant(VT_R4);
    handle->u.f = (float)value;
    return handle;
}

/* Len, UCase, LCase, Trim, LTrim, RTrim and CStr: the text of the value. */
static RtVariant* library_text(const RtVariant* argument, int which) {
    RtString* text;
    RtString* result;
    if (vtype(argument) == VT_NULL) {
        return which == 7 ? variant_fail(94) : variant_null();
    }
    text = variant_render(argument);
    switch (which) {
        case 0:
            result = NULL;
            {
                RtVariant* handle = variant_from_long(length_of(text));
                release(text);
                return handle;
            }
        case 1:
            result = string_upper(text);
            break;
        case 2:
            result = string_lower(text);
            break;
        case 3:
            result = string_trim(text);
            break;
        case 4:
            result = string_ltrim(text);
            break;
        case 5:
            result = string_rtrim(text);
            break;
        default:
            result = retain(text);
            break;
    }
    release(text);
    return variant_from_text(result);
}

/* ----- Library functions with more than one argument */

/* A count or position argument (Left, Mid, Space, String, Choose): Null and
 * text that is not a number are a type mismatch, a fraction rounds to even. */
static int count_argument(const RtVariant* handle, int32_t* into) {
    double parsed;
    switch (vtype(handle)) {
        case VT_NULL:
            raise_error(13);
            return 0;
        case VT_EMPTY:
            *into = 0;
            return 1;
        case VT_BOOL:
            *into = handle->u.l != 0 ? -1 : 0;
            return 1;
        case VT_I2:
        case VT_I4:
        case VT_UI1:
            *into = handle->u.l;
            return 1;
        case VT_R4:
        case VT_R8:
            return round_to_long(number_value(handle), into);
        case VT_BSTR:
            if (parse_numeric(handle->u.s == NULL ? "" : handle->u.s->data,
                              (size_t)length_of(handle->u.s), &parsed)) {
                return round_to_long(parsed, into);
            }
            raise_error(13);
            return 0;
        default:
            raise_error(13);
            return 0;
    }
}

/* Left, Right: which = 0, 1. */
static RtVariant* library_edge(const RtVariant* text_argument,
                               const RtVariant* count_value, int which) {
    int32_t count;
    RtString* text;
    RtString* result;
    if (vtype(text_argument) == VT_NULL && vtype(count_value) == VT_NULL) {
        return variant_null();
    }
    if (!count_argument(count_value, &count)) {
        return NULL;
    }
    if (vtype(text_argument) == VT_NULL) {
        return variant_null();
    }
    if (count < 0) {
        return variant_fail(5);
    }
    text = variant_render(text_argument);
    result = which == 0 ? string_left(text, count) : string_right(text, count);
    release(text);
    return variant_from_text(result);
}

/* Mid(text, start[, length]). */
static RtVariant* library_mid(const RtVariant* text_argument,
                              const RtVariant* start_value,
                              const RtVariant* length_value, int has_length) {
    int32_t start;
    int32_t length = OMITTED;
    RtString* text;
    RtString* result;
    if (vtype(text_argument) == VT_NULL && vtype(start_value) == VT_NULL) {
        return variant_null();
    }
    if (!count_argument(start_value, &start)) {
        return NULL;
    }
    if (has_length && !count_argument(length_value, &length)) {
        return NULL;
    }
    if (vtype(text_argument) == VT_NULL) {
        return variant_null();
    }
    if (start < 1 || (has_length && length < 0)) {
        return variant_fail(5);
    }
    text = variant_render(text_argument);
    result = string_mid(text, start, length);
    release(text);
    return variant_from_text(result);
}

static RtVariant* library_instr(const RtVariant* haystack,
                                const RtVariant* needle) {
    RtString* text;
    RtString* pattern;
    int32_t found = 0;
    if (vtype(haystack) == VT_NULL || vtype(needle) == VT_NULL) {
        return variant_fail(13);
    }
    text = variant_render(haystack);
    pattern = variant_render(needle);
    if (length_of(text) == 0) {
        found = 0;
    } else if (length_of(pattern) == 0) {
        found = 1;
    } else {
        found = string_instr(1, text, pattern);
    }
    release(text);
    release(pattern);
    return variant_from_long(found);
}

static RtVariant* library_strcomp(const RtVariant* left,
                                  const RtVariant* right) {
    RtString* a;
    RtString* b;
    int32_t order;
    if (vtype(left) == VT_NULL || vtype(right) == VT_NULL) {
        return variant_null();
    }
    a = variant_render(left);
    b = variant_render(right);
    order = string_compare(a, b);
    release(a);
    release(b);
    return variant_from_long(order);
}

static RtVariant* library_replace(const RtVariant* source,
                                  const RtVariant* find,
                                  const RtVariant* with) {
    RtString* text;
    RtString* pattern;
    RtString* replacement;
    RtString* result;
    char* buffer;
    int32_t length;
    int32_t want;
    int32_t with_length;
    int32_t at = 0;
    size_t used = 0;
    size_t capacity;
    if (vtype(source) == VT_NULL || vtype(find) == VT_NULL ||
        vtype(with) == VT_NULL) {
        return variant_fail(13);
    }
    text = variant_render(source);
    pattern = variant_render(find);
    replacement = variant_render(with);
    length = length_of(text);
    want = length_of(pattern);
    with_length = length_of(replacement);
    if (want == 0 || length == 0) {
        release(pattern);
        release(replacement);
        return variant_from_text(text);
    }
    capacity = (size_t)length + 1U;
    buffer = (char*)malloc(capacity);
    if (buffer == NULL) {
        die("out of memory");
    }
    while (at < length) {
        if (at + want <= length &&
            memcmp(text->data + at, pattern->data, (size_t)want) == 0) {
            if (used + (size_t)with_length + 1U > capacity) {
                capacity = (used + (size_t)with_length + 1U) * 2U;
                buffer = (char*)realloc(buffer, capacity);
                if (buffer == NULL) {
                    die("out of memory");
                }
            }
            if (with_length > 0) {
                memcpy(buffer + used, replacement->data, (size_t)with_length);
            }
            used += (size_t)with_length;
            at += want;
        } else {
            if (used + 2U > capacity) {
                capacity = (used + 2U) * 2U;
                buffer = (char*)realloc(buffer, capacity);
                if (buffer == NULL) {
                    die("out of memory");
                }
            }
            buffer[used++] = text->data[at++];
        }
    }
    result = make_string(buffer, used);
    free(buffer);
    release(text);
    release(pattern);
    release(replacement);
    return variant_from_text(result);
}

static RtVariant* library_space(const RtVariant* count_value) {
    int32_t count;
    if (!count_argument(count_value, &count)) {
        return NULL;
    }
    if (count < 0) {
        return variant_fail(5);
    }
    return variant_from_text(string_space(count));
}

/* String(count, character): a text gives its first character, a whole number
 * its code. */
static RtVariant* library_string(const RtVariant* count_value,
                                 const RtVariant* character) {
    int32_t count;
    int32_t code;
    char one;
    RtString* result;
    int32_t i;
    if (!count_argument(count_value, &count)) {
        return NULL;
    }
    if (vtype(character) == VT_BSTR) {
        if (length_of(character->u.s) == 0) {
            return variant_fail(5);
        }
        one = character->u.s->data[0];
    } else if (vtype(character) == VT_I2 || vtype(character) == VT_I4 ||
               vtype(character) == VT_UI1) {
        code = character->u.l;
        one = (char)(code & 0xFF);
    } else {
        return variant_fail(13);
    }
    if (count < 0) {
        return variant_fail(5);
    }
    if (count == 0) {
        return variant_from_text(NULL);
    }
    {
        RtString* filled = string_space(count);
        for (i = 0; i < count; ++i) {
            filled->data[i] = one;
        }
        result = filled;
    }
    return variant_from_text(result);
}

static RtVariant* library_iif(const RtVariant* condition,
                              const RtVariant* when_true,
                              const RtVariant* when_false) {
    int32_t answer = variant_to_boolean(condition);
    if (*g_pending != 0) {
        return NULL;
    }
    return variant_retain((RtVariant*)(answer != 0 ? when_true : when_false));
}

static RtVariant* library_choose(const RtVariant* index, int32_t count,
                                 const char* block) {
    int32_t chosen;
    if (!count_argument(index, &chosen)) {
        return NULL;
    }
    if (chosen < 1 || chosen >= count) {
        return variant_null();
    }
    return variant_retain(argument_at(block, chosen));
}

static int name_is(const RtString* name, const char* wanted) {
    size_t size = strlen(wanted);
    size_t have = (size_t)length_of(name);
    if (have > 0U && name->data[have - 1U] == '$') {
        --have; /* Left$ is Left */
    }
    return have == size && memcmp(name->data, wanted, size) == 0;
}

static RtVariant* variant_call(const RtString* name, int32_t count,
                               const char* block) {
    RtVariant* first = count > 0 ? argument_at(block, 0) : NULL;
    if (count == 1) {
        if (name_is(name, "abs")) {
            return library_round(first, 0);
        }
        if (name_is(name, "int")) {
            return library_round(first, 1);
        }
        if (name_is(name, "fix")) {
            return library_round(first, 2);
        }
        if (name_is(name, "sgn")) {
            return library_sgn(first);
        }
        if (name_is(name, "sqr")) {
            return library_sqr(first);
        }
        if (name_is(name, "cint")) {
            return library_convert_whole(first, 0);
        }
        if (name_is(name, "clng")) {
            return library_convert_whole(first, 1);
        }
        if (name_is(name, "cbyte")) {
            return library_convert_whole(first, 2);
        }
        if (name_is(name, "cdbl")) {
            return library_cdbl(first, 0);
        }
        if (name_is(name, "csng")) {
            return library_cdbl(first, 1);
        }
        if (name_is(name, "cstr")) {
            return library_text(first, 7);
        }
        if (name_is(name, "cbool")) {
            if (vtype(first) == VT_NULL) {
                return variant_fail(94);
            }
            {
                int32_t answer = variant_to_boolean(first);
                if (*g_pending != 0) {
                    return NULL;
                }
                return variant_from_boolean(answer);
            }
        }
        if (name_is(name, "len")) {
            return library_text(first, 0);
        }
        if (name_is(name, "ucase")) {
            return library_text(first, 1);
        }
        if (name_is(name, "lcase")) {
            return library_text(first, 2);
        }
        if (name_is(name, "trim")) {
            return library_text(first, 3);
        }
        if (name_is(name, "ltrim")) {
            return library_text(first, 4);
        }
        if (name_is(name, "rtrim")) {
            return library_text(first, 5);
        }
    }
    if (count == 2) {
        if (name_is(name, "left")) {
            return library_edge(first, argument_at(block, 1), 0);
        }
        if (name_is(name, "right")) {
            return library_edge(first, argument_at(block, 1), 1);
        }
        if (name_is(name, "mid")) {
            return library_mid(first, argument_at(block, 1), NULL, 0);
        }
        if (name_is(name, "instr")) {
            return library_instr(first, argument_at(block, 1));
        }
        if (name_is(name, "strcomp")) {
            return library_strcomp(first, argument_at(block, 1));
        }
        if (name_is(name, "string")) {
            return library_string(first, argument_at(block, 1));
        }
    }
    if (count == 3) {
        if (name_is(name, "mid")) {
            return library_mid(first, argument_at(block, 1),
                               argument_at(block, 2), 1);
        }
        if (name_is(name, "replace")) {
            return library_replace(first, argument_at(block, 1),
                                   argument_at(block, 2));
        }
        if (name_is(name, "iif")) {
            return library_iif(first, argument_at(block, 1),
                               argument_at(block, 2));
        }
    }
    if (count >= 2 && name_is(name, "choose")) {
        return library_choose(first, count, block);
    }
    if (count == 1 && name_is(name, "space")) {
        return library_space(first);
    }
    die("wfc: a library function of the program is not in this runtime");
    return NULL;
}

static void fill_table(void) {
    int i;
    for (i = 0; i < SERVICE_COUNT; ++i) {
        g_table[i] = (void*)unsupported;
    }
    g_table[0] = (void*)print_number;
    g_table[1] = (void*)print_boolean;
    g_table[2] = (void*)print_text;
    g_table[3] = (void*)print_newline;
    g_table[4] = (void*)raise_error;
    g_table[5] = (void*)end_program;
    g_table[6] = (void*)print_string;
    g_table[7] = (void*)string_concat;
    g_table[8] = (void*)string_assign;
    g_table[9] = (void*)string_release;
    g_table[10] = (void*)string_addref;
    g_table[11] = (void*)string_length;
    g_table[12] = (void*)string_compare;
    g_table[13] = (void*)string_left;
    g_table[14] = (void*)string_right;
    g_table[15] = (void*)string_mid;
    g_table[16] = (void*)string_upper;
    g_table[17] = (void*)string_lower;
    g_table[18] = (void*)string_trim;
    g_table[19] = (void*)string_ltrim;
    g_table[20] = (void*)string_rtrim;
    g_table[21] = (void*)string_chr;
    g_table[22] = (void*)string_asc;
    g_table[23] = (void*)string_from_number;
    g_table[24] = (void*)string_from_integer;
    g_table[25] = (void*)string_from_boolean;
    g_table[26] = (void*)string_instr;
    g_table[27] = (void*)string_space;
    g_table[28] = (void*)print_double;
    g_table[29] = (void*)string_from_double;
    g_table[30] = (void*)double_to_long;
    g_table[31] = (void*)math_function;
    g_table[32] = (void*)double_sign;
    g_table[33] = (void*)array_redim;
    g_table[34] = (void*)array_element1;
    g_table[35] = (void*)array_element2;
    g_table[36] = (void*)array_element3;
    g_table[37] = (void*)array_bound;
    g_table[38] = (void*)array_release;
    g_table[39] = (void*)array_erase;
    g_table[40] = (void*)error_number;
    g_table[41] = (void*)error_description;
    g_table[42] = (void*)error_source;
    g_table[43] = (void*)error_clear;
    g_table[44] = (void*)error_raise;
    g_table[45] = (void*)error_acknowledge;
    g_table[46] = (void*)print_zone;
    g_table[47] = (void*)double_power;
    g_table[48] = (void*)variant_from_long;
    g_table[49] = (void*)variant_from_integer;
    g_table[50] = (void*)variant_from_double;
    g_table[51] = (void*)variant_from_single;
    g_table[52] = (void*)variant_from_string;
    g_table[53] = (void*)variant_from_boolean;
    g_table[54] = (void*)variant_null;
    g_table[55] = (void*)variant_assign;
    g_table[56] = (void*)variant_release;
    g_table[57] = (void*)variant_addref;
    g_table[58] = (void*)variant_binary;
    g_table[59] = (void*)variant_negate;
    g_table[60] = (void*)variant_not;
    g_table[61] = (void*)variant_to_long;
    g_table[62] = (void*)variant_to_double;
    g_table[63] = (void*)variant_to_string;
    g_table[64] = (void*)variant_to_boolean;
    g_table[65] = (void*)variant_print;
    g_table[66] = (void*)variant_type_name;
    g_table[67] = (void*)variant_var_type;
    g_table[68] = (void*)variant_is_numeric;
    g_table[77] = (void*)variant_call;
    g_table[78] = (void*)variant_missing;
    g_table[79] = (void*)variant_is_missing;
    g_table[90] = (void*)variant_from_byte;
    g_table[80] = (void*)array_count;
    g_table[81] = (void*)array_flat_element;
    g_table[89] = (void*)string_mid_assign;
    g_table[103] = (void*)array_assign;
    g_table[107] = (void*)string_fixed;
    g_table[108] = (void*)trace_call;
    g_table[112] = (void*)print_tab;
    g_table[113] = (void*)print_spc;
    g_table[115] = (void*)error_raise_help;
    g_table[116] = (void*)error_help_file;
    g_table[117] = (void*)error_help_context;
}

/* ----- Finding and loading the program
 * ---------------------------------------------- */

typedef struct {
    const unsigned char* at;
    size_t left;
} Cursor;

static int take(Cursor* cursor, void* into, size_t size) {
    if (cursor->left < size) {
        return 0;
    }
    memcpy(into, cursor->at, size);
    cursor->at += size;
    cursor->left -= size;
    return 1;
}

static int skip(Cursor* cursor, size_t size) {
    if (cursor->left < size) {
        return 0;
    }
    cursor->at += size;
    cursor->left -= size;
    return 1;
}

/* A length-prefixed text: the position and the length. */
static int take_text(Cursor* cursor, const unsigned char** text,
                     uint32_t* length) {
    if (!take(cursor, length, 4U) || cursor->left < *length) {
        return 0;
    }
    *text = cursor->at;
    return skip(cursor, *length);
}

static unsigned char* read_self(size_t* size) {
    char path[1024];
    HANDLE file;
    DWORD length;
    DWORD done = 0;
    unsigned char* bytes;
    if (GetModuleFileNameA(NULL, path, sizeof(path)) == 0UL) {
        return NULL;
    }
    file = CreateFileA(path, GENERIC_READ_, FILE_SHARE_READ_, NULL,
                       OPEN_EXISTING_, 0UL, NULL);
    if (file == (HANDLE)-1) {
        return NULL;
    }
    length = GetFileSize(file, NULL);
    bytes = (unsigned char*)malloc(length);
    if (bytes == NULL || !ReadFile(file, bytes, length, &done, NULL) ||
        done != length) {
        CloseHandle(file);
        return NULL;
    }
    CloseHandle(file);
    *size = length;
    return bytes;
}

/* The payload of a packaged program (src/package.cpp): the options and the
 * files. Looks for the file `__wfc_native.bin` and returns it. */
static const unsigned char* find_image(const unsigned char* all, size_t size,
                                       size_t* image_size) {
    uint64_t payload_size;
    Cursor cursor;
    uint32_t version;
    uint32_t count;
    uint32_t i;
    const unsigned char* text;
    uint32_t length;
    if (size < 16U || memcmp(all + size - 16U, "WFCPKG01", 8U) != 0) {
        return NULL;
    }
    memcpy(&payload_size, all + size - 8U, 8U);
    if (payload_size > size - 16U) {
        return NULL;
    }
    cursor.at = all + size - 16U - (size_t)payload_size;
    cursor.left = (size_t)payload_size;
    if (!take(&cursor, &version, 4U) || version != 1U ||
        !take(&cursor, &count, 4U)) {
        return NULL;
    }
    for (i = 0; i < count; ++i) { /* the options */
        if (!take_text(&cursor, &text, &length)) {
            return NULL;
        }
    }
    if (!take_text(&cursor, &text, &length) || !take(&cursor, &count, 4U)) {
        return NULL; /* the project file name, the file count */
    }
    for (i = 0; i < count; ++i) {
        uint64_t content;
        if (!take_text(&cursor, &text, &length) ||
            !take(&cursor, &content, 8U) || cursor.left < content) {
            return NULL;
        }
        if (length == 16U && memcmp(text, "__wfc_native.bin", 16U) == 0) {
            *image_size = (size_t)content;
            return cursor.at;
        }
        if (!skip(&cursor, (size_t)content)) {
            return NULL;
        }
    }
    return NULL;
}

#define IMAGE_VERSION 1U
#define RUNTIME_API_VERSION 2U
#define ARCHITECTURE_X86 0U

typedef void (*EntryFunction)(void);

static void run_image(const unsigned char* bytes, size_t size) {
    Cursor cursor;
    uint32_t version;
    uint32_t architecture;
    uint32_t api;
    uint64_t table_count;
    uint64_t entry;
    uint64_t flag;
    uint32_t classes;
    uint32_t i;
    uint64_t image_size;
    unsigned char* memory;
    DWORD previous = 0;
    MEMORY_BASIC_INFORMATION info;
    uintptr_t here;
    uintptr_t limit;

    cursor.at = bytes;
    cursor.left = size;
    if (!skip(&cursor, 8U) || !take(&cursor, &version, 4U) ||
        !take(&cursor, &architecture, 4U) || !take(&cursor, &api, 4U) ||
        !take(&cursor, &table_count, 8U) || !take(&cursor, &entry, 8U) ||
        !take(&cursor, &flag, 8U) || !take(&cursor, &classes, 4U) ||
        memcmp(bytes, "WFCNATIV", 8U) != 0 || version != IMAGE_VERSION ||
        api != RUNTIME_API_VERSION || architecture != ARCHITECTURE_X86 ||
        table_count != SERVICE_COUNT) {
        die("wfc: the program was made for another version of WFC");
    }
    for (i = 0; i < classes; ++i) {
        const unsigned char* name;
        uint32_t length;
        uint64_t record;
        if (!take_text(&cursor, &name, &length) ||
            !take(&cursor, &record, 8U)) {
            die("wfc: the program is damaged");
        }
    }
    if (!take(&cursor, &image_size, 8U) || cursor.left < image_size ||
        image_size < table_count * 4U) {
        die("wfc: the program is damaged");
    }
    memory = (unsigned char*)VirtualAlloc(NULL, (size_t)image_size,
                                          MEM_COMMIT_RESERVE_, PAGE_READWRITE_);
    if (memory == NULL) {
        die("wfc: cannot allocate memory for the program");
    }
    memcpy(memory, cursor.at, (size_t)image_size);
    fill_table();
    memcpy(memory, g_table, SERVICE_COUNT * sizeof(void*));
    if (!VirtualProtect(memory, (size_t)image_size, PAGE_EXECUTE_READWRITE_,
                        &previous)) {
        die("wfc: cannot make the program executable");
    }
    FlushInstructionCache(GetCurrentProcess(), memory, (size_t)image_size);

    /* The stack limit: half of the stack that is left (the services need some
     * room too). The stack is one allocation: its base is the low end. */
    here = (uintptr_t)&info;
    if (VirtualQuery(&info, &info, sizeof(info)) != 0U) {
        uintptr_t low = (uintptr_t)info.AllocationBase;
        uintptr_t remaining = here > low ? here - low : 0U;
        limit = here - remaining / 2U;
        memcpy(memory + flag + 8U, &limit, sizeof(limit));
    }
    g_pending = (int32_t*)(memory + flag);
    ((EntryFunction)(memory + entry))();
    finish_output();
    if (*g_pending != 0) {
        /* An error that no handler took ends the program. */
        fprintf(stderr, "Run-time error '%ld': %s\n", (long)g_error_number,
                g_error_description);
        fflush(stderr);
        exit(1);
    }
}

int main(void) {
    size_t size = 0;
    size_t image_size = 0;
    unsigned char* all = read_self(&size);
    const unsigned char* image;
    if (all == NULL) {
        die("wfc: cannot read the program");
    }
    image = find_image(all, size, &image_size);
    if (image == NULL) {
        die("wfc: this file carries no program");
    }
    run_image(image, image_size);
    return 0;
}
