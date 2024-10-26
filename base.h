#ifndef BASE_BASE
#define BASE_BASE

#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#define COMPILER_GCC 1
#elif defined(__clang__) && !defined(_MSC_VER)
#define COMPILER_CLANG 1
#elif defined(_MSC_VER)
#define COMPILER_CL 1
#if defined(_M_IX86)
#define ARCH_X86 1
#elif defined(_M_AMD64)
#define ARCH_X64 1
#elif defined(_M_ARM)
#define ARCH_ARM 1
#elif defined(_M_ARM64)
#define ARCH_ARM64 1
#else
#error "Unsupported platform"
#endif
#else
#error "Unsupported compiler"
#endif

#if defined(__gnu_linux__)
#define OS_LINUX 1
#elif defined(__unix__)
#define OS_BSD 1
#elif defined(_WIN32)
#define OS_WINDOWS 1
#elif defined(__APPLE__)
#define OS_MAC 1
#else
#error "Unsupported OS"
#endif

#if defined(__i386__)
#define ARCH_X86 1
#elif defined(__x86_64__)
#define ARCH_X64 1
#elif defined(__arm__)
#define ARCH_ARM 1
#elif defined(__aarch64__)
#define ARCH_ARM64 1
#else
#error "Unsupported platform"
#endif

#if !defined(COMPILER_GCC)
#define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CLANG)
#define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_CL)
#define COMPILER_CL 0
#endif

#if !defined(OS_LINUX)
#define OS_LINUX 0
#endif
#if !defined(OS_BSD)
#define OS_BSD 0
#endif
#if !defined(OS_MAC)
#define OS_MAC 0
#endif
#if !defined(OS_WINDOWS)
#define OS_WINDOWS 0
#endif

#if !defined(ARCH_X86)
#define ARCH_X86 0
#endif
#if !defined(ARCH_X64)
#define ARCH_X64 0
#endif
#if !defined(ARCH_ARM)
#define ARCH_ARM 0
#endif
#if !defined(ARCH_ARM64)
#define ARCH_ARM64 0
#endif

#define _stmt(S)                                                               \
  do {                                                                         \
    S                                                                          \
  } while (0)

#ifndef _assert_break
#if OS_WINDOWS
#define _assert_break() __debugbreak()
#else
#define _assert_break() __builtin_trap()
#endif
#endif

#ifdef ENABLE_ASSERT
#define Assert(COND) _stmt(if (!(COND)) { _assert_break(); })
#else
#define Assert(COND)
#endif

#define Arrsize(ARR) (sizeof((ARR)) / sizeof(*(ARR)))
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#define Min(a, b) ((a) <= (b) ? (a) : (b))
#define ClampTop(a, b) Min((a), (b))
#define ClampBot(a, b) Max((a), (b))

#define kiB(BYTES) ((BYTES)*1024)
#define MB(BYTES) (kiB((BYTES)) * 1024)
#define GB(BYTES) (MB((BYTES)) * 1024UL)
#define TB(BYTES) (GB((BYTES)) * 1024UL)

#define global static
#define local static
#define fn static

#define export_c extern "C"
#define begin_export_c extern "C" {
#define end_export_c }

#if COMPILER_GCC || COMPILER_CLANG
#define shared_export export_c __attribute__((visibility("default")))
#elif COMPILER_CL
#define shared_export export_c __declspec(dllexport)
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

typedef enum {
  false,
  true
} bool;

#if defined(ARCH_X64) || defined(ARCH_ARM64)
typedef u64 size_t;
#else
typedef u32 size_t;
#endif

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

#endif
