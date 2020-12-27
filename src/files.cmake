set(HEADERS
    #SSIG
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/algorithm.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/atomic.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/exception.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/function_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/functional.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/intrusive_ptr.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/mutex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/tuple.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/type_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/utility.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/vector.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/concept/signal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/signal.hpp
    )

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/concept/connection_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/impl/signal_impl.hpp
    )

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sywu.cpp
    )
