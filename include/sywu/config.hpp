#ifndef SYWU_CONFIG_HPP
#define SYWU_CONFIG_HPP

// Compiler checks.
#if defined(__clang__)
#   if defined(__apple_build_version__)
#       /* http://en.wikipedia.org/wiki/Xcode#Toolchain_Versions */
#       if __apple_build_version__ >= 7000053
#           define CC_CLANG 306
#       elif __apple_build_version__ >= 6000051
#           define CC_CLANG 305
#       elif __apple_build_version__ >= 5030038
#           define CC_CLANG 304
#       elif __apple_build_version__ >= 5000275
#           define CC_CLANG 303
#       elif __apple_build_version__ >= 4250024
#           define CC_CLANG 302
#       elif __apple_build_version__ >= 3180045
#           define CC_CLANG 301
#       elif __apple_build_version__ >= 2111001
#           define CC_CLANG 300
#       else
#           error "Unknown Apple Clang version"
#       endif
#   define SYWU_LONG_SYNONIM_OF_UINT64
#   else
#       define CC_CLANG ((__clang_major__ * 100) + __clang_minor__)
#   endif
#   define SYWU_EXCEPTION_NOEXCEPT          _NOEXCEPT
#   define SYWU_DECLARE_NOEXCEPT            _NOEXCEPT
#   define SYWU_DECLARE_NOEXCEPTX(x)
#elif defined(__GNUC__) || defined(__GLIBCXX__)
#   define SYWU_EXCEPTION_NOEXCEPT          _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_USE_NOEXCEPT
#   define SYWU_DECLARE_NOEXCEPT            _GLIBCXX_USE_NOEXCEPT
#   define SYWU_DECLARE_NOEXCEPTX(x)        noexcept(x)
#endif // GNUC


// OSX and Linux use the same declaration mode for inport and export
#define SYWU_DECL_EXPORT         __attribute__((visibility("default")))
#define SYWU_DECL_IMPORT         __attribute__((visibility("default")))

#define SYWU_FALLTHROUGH     [[fallthrough]]

// unused parameters
#define SYWU_UNUSED(x)       (void)x

//
// disable copy construction and operator
//
#define SYWU_DISABLE_COPY(Class) \
    Class(const Class&) = delete;\
    Class& operator=(const Class&) = delete;
#define SYWU_DISABLE_MOVE(Class) \
    Class(Class&&) = delete; \
    Class& operator=(Class&&) = delete;

#define SYWU_DISABLE_COPY_OR_MOVE(Class) \
    SYWU_DISABLE_COPY(Class) \
    SYWU_DISABLE_MOVE(Class)

#ifdef SYWU_CONFIG_LIBRARY
#   define SYWU_API     SYWU_DECL_EXPORT
#else
#   define SYWU_API     SYWU_DECL_IMPORT
#endif
#define SYWU_TEMPLATE_API

#ifdef DEBUG
#include <cassert>
#define SYWU_ASSERT(test)    if (!(test)) abort()
#else
#define SYWU_ASSERT(test)    (void)(test)
#endif

#endif // SYWU_CONFIG_HPP
