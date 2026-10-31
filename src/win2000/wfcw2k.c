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
            return "Sub, Function, or Property not defined";
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

/* VB6 writes a Double with 15 significant digits and a Single with 7 (C's %G
 * rules); negative zero is "0". */
static void format_float(char* buffer, size_t size, double value, int digits) {
    if (value != value) {
        copy_text(buffer, "nan", size);
        return;
    }
    if (value > 1.7976931348623157e308 || value < -1.7976931348623157e308) {
        copy_text(buffer, value > 0 ? "inf" : "-inf", size);
        return;
    }
    sprintf(buffer, "%.*G", digits, value);
    if (strcmp(buffer, "-0") == 0) {
        copy_text(buffer, "0", size);
    }
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
    if (base == 0.0 && exponent < 0.0) {
        raise_error(11);
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
                    unsupported();
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
    } else if (array->kind == 4 || array->kind == 5) {
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
                    unsupported();
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
    } else if (array->kind == 4 || array->kind == 5) {
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
    if (kind > 3) {
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
                if (kind == 2) {
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
