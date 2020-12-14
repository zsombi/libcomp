set(HEADERS
    #SSIG
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/extras.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/type_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/guards.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/signal.hpp
    )

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/sywu/impl/signal_impl.hpp
    )

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sywu.cpp
    )
