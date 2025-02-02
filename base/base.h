#ifndef BASE_BASE
#define BASE_BASE

#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#  define COMPILER_GCC 1
#elif defined(__clang__) && !defined(_MSC_VER)
#  define COMPILER_CLANG 1
#elif defined(_MSC_VER)
#  define COMPILER_CL 1
#  if defined(_M_IX86)
#    define ARCH_X86 1
#  elif defined(_M_AMD64)
#    define ARCH_X64 1
#  elif defined(_M_ARM)
#    define ARCH_ARM 1
#  elif defined(_M_ARM64)
#    define ARCH_ARM64 1
#  else
#    error "Unsupported platform"
#  endif
#else
#  error "Unsupported compiler"
#endif

#if defined(__gnu_linux__)
#  define OS_LINUX 1
#elif defined(__unix__)
#  define OS_BSD 1
#elif defined(_WIN32)
#  define OS_WINDOWS 1
#elif defined(__APPLE__)
#  define OS_MAC 1
#else
#  error "Unsupported OS"
#endif

#if defined(__cplusplus)
#  define CPP 1
#else
#  define CPP 0
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64)
#  define ARCH_X64 1
#elif defined(i386) || defined(__i386) || defined(__i386__)
#  define ARCH_X86 1
#elif defined(__aarch64__)
#  define ARCH_ARM64 1
#elif defined(__arm__)
#  define ARCH_ARM32 1
#else
#  error "Unsopported platform"
#endif

#if !defined(COMPILER_GCC)
#  define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CLANG)
#  define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_CL)
#  define COMPILER_CL 0
#endif

#if !defined(OS_LINUX)
#  define OS_LINUX 0
#endif
#if !defined(OS_BSD)
#  define OS_BSD 0
#endif
#if !defined(OS_MAC)
#  define OS_MAC 0
#endif
#if !defined(OS_WINDOWS)
#  define OS_WINDOWS 0
#endif

#if !defined(ARCH_X86)
#  define ARCH_X86 0
#endif
#if !defined(ARCH_X64)
#  define ARCH_X64 0
#endif
#if !defined(ARCH_ARM)
#  define ARCH_ARM 0
#endif
#if !defined(ARCH_ARM64)
#  define ARCH_ARM64 0
#endif

#if defined(DEBUG)
#  undef DEBUG
#  define DEBUG 1
#  define NDEBUG 0
#else
#  define DEBUG 0
#  define NDEBUG 1
#endif

#define TLS_CTX_SIZE MB(64)

#if COMPILER_GCC
#  define AlignOf(TYPE) __alignof__(TYPE)
#elif COMPILER_CLANG
#  define AlignOf(TYPE) _Alignof(TYPE)
#elif COMPILER_CL
#  define AlignOf(TYPE) __alignof(TYPE)
#else
#  define AlignOf(TYPE) 1
#endif

#if COMPILER_CL
#  define threadvar __declspec(thread)
#elif COMPILER_CLANG || COMPILER_GCC
#  define threadvar __thread
#endif


#define _stmt(S)                                                               \
  do {                                                                         \
    S                                                                          \
  } while (0)

#ifndef _assert_break
#  if OS_WINDOWS
#    define _assert_break() __debugbreak()
#  else
#    define _assert_break() __builtin_trap()
#  endif
#endif

#define AssertAlways(COND) _stmt(if (!(COND)) { _assert_break(); })
#define AssertAlwaysWithMsg(COND, MSG) _stmt(\
  if (!(COND)) {\
    printf("%s", MSG);\
    _assert_break(); \
  })
#define StaticAssert(C, ID) global u8 Glue(ID, __LINE__)[(C)?1:-1]

#ifdef ENABLE_ASSERT
#  define Assert(COND) AssertAlways(COND)
#  define AssertMsg(COND, MSG) AssertAlwaysWithMsg(COND, MSG)
#else
#  define Assert(COND) (void)(COND)
#endif

#define Stringify_(S) (#S)
#define Stringify(S) Stringify_(S)

#define Glue_(A, B) (A##B)
#define Glue(A, B) Glue_(A,B)

#define Arrsize(ARR) (sizeof((ARR)) / sizeof(*(ARR)))
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#define Min(a, b) ((a) <= (b) ? (a) : (b))
#define ClampTop(a, b) Min((a), (b))
#define ClampBot(a, b) Max((a), (b))
#define Swap(a, b, _tmp) ((_tmp) = (a), (a) = (b), (b) = (_tmp))
#define Abs(a) ((a) >= 0 ? (a) : (-(a)))

#define DeferLoop(...) for(u8 __i_ = 1; __i_; --__i_, __VA_ARGS__)

#define Not(A) (~(A))
#define And(A, B) ((A) & (B))
#define Or(A, B) ((A) | (B))
#define Nand(A, B) (~((A) && (B)))
#define Nor(A, B) (~((A) || (B)))
#define Xor(A, B) ((A) ^ (B))
#define Xnor(A, B) (~(A) ^ (B))
#define GetBit(NUM, I) ((NUM & (1 << I)) >> I)
#define SetBit(NUM, I, BIT) (NUM | (1 << (BIT - 1)))

#define KiB(BYTES) ((BYTES)*1024)
#define MB(BYTES) (KiB((BYTES)) * 1024)
#define GB(BYTES) (MB((BYTES)) * 1024UL)
#define TB(BYTES) (GB((BYTES)) * 1024UL)

#define global static
#define local static
#define fn static

#define CExport extern "C"
#define CExportBegin extern "C" {
#define CExportEnd }

#if COMPILER_GCC || COMPILER_CLANG
#  define DLLExport export_c __attribute__((visibility("default")))
#elif COMPILER_CL
#  define DLLExport export_c __declspec(dllexport)
#endif

#include <stdint.h>
typedef float f32;
typedef double f64;
typedef long double f128;

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

#if !CPP
typedef enum {
  false,
  true
} bool;
#endif

#if defined(ARCH_X64) || defined(ARCH_ARM64)
  typedef u64 usize;
  typedef i64 isize;
#else
  typedef u32 usize;
  typedef i32 isize;
#endif

#define DefaultAlignment (2 * sizeof(void*))

#define U8_MAX 0xFF
#define U8_MIN 0
#define U16_MAX 0xFFFF
#define U16_MIN 0
#define U32_MAX 0xFFFFFFFF
#define U32_MIN 0
#define U64_MAX 0xFFFFFFFFFFFFFFFF
#define U64_MIN 0

#define I8_MAX 0x7F
#define I8_MIN (-0x80)
#define I16_MAX 0x7FFF
#define I16_MIN (-0x8000)
#define I32_MAX 0x7FFFFFFF
#define I32_MIN (-0x80000000)
#define I64_MAX 0x7FFFFFFFFFFFFFFF
#define I64_MIN (-0x8000000000000000)

#if defined(ARCH_X64) || defined(ARCH_ARM64)
#  define USIZE_MAX U64_MAX
#  define USIZE_MIN U64_MIN
#  define ISIZE_MAX I64_MAX
#  define ISIZE_MIN I64_MIN
#else
#  define USIZE_MAX U32_MAX
#  define USIZE_MIN U32_MIN
#  define ISIZE_MAX I32_MAX
#  define ISIZE_MIN I32_MIN
#endif

global const u32 bitmask1  = 0x00000001;
global const u32 bitmask2  = 0x00000003;
global const u32 bitmask3  = 0x00000007;
global const u32 bitmask4  = 0x0000000f;
global const u32 bitmask5  = 0x0000001f;
global const u32 bitmask6  = 0x0000003f;
global const u32 bitmask7  = 0x0000007f;
global const u32 bitmask8  = 0x000000ff;
global const u32 bitmask9  = 0x000001ff;
global const u32 bitmask10 = 0x000003ff;
global const u32 bitmask11 = 0x000007ff;
global const u32 bitmask12 = 0x00000fff;
global const u32 bitmask13 = 0x00001fff;
global const u32 bitmask14 = 0x00003fff;
global const u32 bitmask15 = 0x00007fff;
global const u32 bitmask16 = 0x0000ffff;
global const u32 bitmask17 = 0x0001ffff;
global const u32 bitmask18 = 0x0003ffff;
global const u32 bitmask19 = 0x0007ffff;
global const u32 bitmask20 = 0x000fffff;
global const u32 bitmask21 = 0x001fffff;
global const u32 bitmask22 = 0x003fffff;
global const u32 bitmask23 = 0x007fffff;
global const u32 bitmask24 = 0x00ffffff;
global const u32 bitmask25 = 0x01ffffff;
global const u32 bitmask26 = 0x03ffffff;
global const u32 bitmask27 = 0x07ffffff;
global const u32 bitmask28 = 0x0fffffff;
global const u32 bitmask29 = 0x1fffffff;
global const u32 bitmask30 = 0x3fffffff;
global const u32 bitmask31 = 0x7fffffff;
global const u32 bitmask32 = 0xffffffff;

global const u64 bitmask33 = 0x00000001ffffffffull;
global const u64 bitmask34 = 0x00000003ffffffffull;
global const u64 bitmask35 = 0x00000007ffffffffull;
global const u64 bitmask36 = 0x0000000fffffffffull;
global const u64 bitmask37 = 0x0000001fffffffffull;
global const u64 bitmask38 = 0x0000003fffffffffull;
global const u64 bitmask39 = 0x0000007fffffffffull;
global const u64 bitmask40 = 0x000000ffffffffffull;
global const u64 bitmask41 = 0x000001ffffffffffull;
global const u64 bitmask42 = 0x000003ffffffffffull;
global const u64 bitmask43 = 0x000007ffffffffffull;
global const u64 bitmask44 = 0x00000fffffffffffull;
global const u64 bitmask45 = 0x00001fffffffffffull;
global const u64 bitmask46 = 0x00003fffffffffffull;
global const u64 bitmask47 = 0x00007fffffffffffull;
global const u64 bitmask48 = 0x0000ffffffffffffull;
global const u64 bitmask49 = 0x0001ffffffffffffull;
global const u64 bitmask50 = 0x0003ffffffffffffull;
global const u64 bitmask51 = 0x0007ffffffffffffull;
global const u64 bitmask52 = 0x000fffffffffffffull;
global const u64 bitmask53 = 0x001fffffffffffffull;
global const u64 bitmask54 = 0x003fffffffffffffull;
global const u64 bitmask55 = 0x007fffffffffffffull;
global const u64 bitmask56 = 0x00ffffffffffffffull;
global const u64 bitmask57 = 0x01ffffffffffffffull;
global const u64 bitmask58 = 0x03ffffffffffffffull;
global const u64 bitmask59 = 0x07ffffffffffffffull;
global const u64 bitmask60 = 0x0fffffffffffffffull;
global const u64 bitmask61 = 0x1fffffffffffffffull;
global const u64 bitmask62 = 0x3fffffffffffffffull;
global const u64 bitmask63 = 0x7fffffffffffffffull;
global const u64 bitmask64 = 0xffffffffffffffffull;

global const u32 bit1  = (1<<0);
global const u32 bit2  = (1<<1);
global const u32 bit3  = (1<<2);
global const u32 bit4  = (1<<3);
global const u32 bit5  = (1<<4);
global const u32 bit6  = (1<<5);
global const u32 bit7  = (1<<6);
global const u32 bit8  = (1<<7);
global const u32 bit9  = (1<<8);
global const u32 bit10 = (1<<9);
global const u32 bit11 = (1<<10);
global const u32 bit12 = (1<<11);
global const u32 bit13 = (1<<12);
global const u32 bit14 = (1<<13);
global const u32 bit15 = (1<<14);
global const u32 bit16 = (1<<15);
global const u32 bit17 = (1<<16);
global const u32 bit18 = (1<<17);
global const u32 bit19 = (1<<18);
global const u32 bit20 = (1<<19);
global const u32 bit21 = (1<<20);
global const u32 bit22 = (1<<21);
global const u32 bit23 = (1<<22);
global const u32 bit24 = (1<<23);
global const u32 bit25 = (1<<24);
global const u32 bit26 = (1<<25);
global const u32 bit27 = (1<<26);
global const u32 bit28 = (1<<27);
global const u32 bit29 = (1<<28);
global const u32 bit30 = (1<<29);
global const u32 bit31 = (1<<30);
global const u32 bit32 = (1<<31);

global const u64 bit33 = (1ull<<32);
global const u64 bit34 = (1ull<<33);
global const u64 bit35 = (1ull<<34);
global const u64 bit36 = (1ull<<35);
global const u64 bit37 = (1ull<<36);
global const u64 bit38 = (1ull<<37);
global const u64 bit39 = (1ull<<38);
global const u64 bit40 = (1ull<<39);
global const u64 bit41 = (1ull<<40);
global const u64 bit42 = (1ull<<41);
global const u64 bit43 = (1ull<<42);
global const u64 bit44 = (1ull<<43);
global const u64 bit45 = (1ull<<44);
global const u64 bit46 = (1ull<<45);
global const u64 bit47 = (1ull<<46);
global const u64 bit48 = (1ull<<47);
global const u64 bit49 = (1ull<<48);
global const u64 bit50 = (1ull<<49);
global const u64 bit51 = (1ull<<50);
global const u64 bit52 = (1ull<<51);
global const u64 bit53 = (1ull<<52);
global const u64 bit54 = (1ull<<53);
global const u64 bit55 = (1ull<<54);
global const u64 bit56 = (1ull<<55);
global const u64 bit57 = (1ull<<56);
global const u64 bit58 = (1ull<<57);
global const u64 bit59 = (1ull<<58);
global const u64 bit60 = (1ull<<59);
global const u64 bit61 = (1ull<<60);
global const u64 bit62 = (1ull<<61);
global const u64 bit63 = (1ull<<62);
global const u64 bit64 = (1ull<<63);

#endif
