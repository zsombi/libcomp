#ifndef API_DEFS_HPP
#define API_DEFS_HPP

#include <config.hpp>

#ifdef CONFIG_LIBRARY
#   define SYWU_API     DECL_EXPORT
#else
#   define SYWU_API     DECL_IMPORT
#endif
#define SYWU_TEMPLATE_API

#endif // API_DEFS_HPP
