#ifndef COMP_EXCEPTION_HPP
#define COMP_EXCEPTION_HPP

#include <exception>
#include <comp/config.hpp>

namespace comp
{

using std::exception;
using std::terminate;

/// Exception thrown when a slot that is not connected is activated.
class COMP_API bad_slot : public exception
{
public:
    explicit bad_slot() = default;
};

}

#endif // COMP_EXCEPTION_HPP
