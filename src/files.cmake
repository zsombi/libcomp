set(HEADERS
    #SSIG
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/algorythm.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/atomic.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/functional.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/mutex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/tuple.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/type_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/utility.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/wrap/vector.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/connection.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/signal.hpp
    )

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/impl/connection_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/impl/signal_impl.hpp
    )

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sywu.cpp
    )
