set(HEADERS
    #SSIG
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/algorithm.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/atomic.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/exception.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/function_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/functional.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/intrusive_ptr.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/mutex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/tuple.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/type_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/utility.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/wrap/vector.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/concept/signal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/signal.hpp
    )

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/concept/slot_concept_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/comp/concept/signal_concept_impl.hpp
    )

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/comp_lib.cpp
    )
