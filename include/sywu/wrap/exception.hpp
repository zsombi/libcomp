#ifndef SYWU_EXCEPTION_HPP
#define SYWU_EXCEPTION_HPP

#include <exception>
#include <sywu/config.hpp>

namespace sywu
{

using std::exception;

class SYWU_API bad_slot : public exception
{
public:
    explicit bad_slot() = default;
};

}

#endif // SYWU_EXCEPTION_HPP
