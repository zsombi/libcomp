#include <comp/signal.hpp>
#include <iostream>

void function()
{
    std::puts("function invoked");
}

struct Functor
{
    void operator()()
    {
        std::puts("Functor invoked");
    }
};

struct Object : public comp::enable_shared_from_this<Object>
{
    void method()
    {
        std::puts("Method invoked");
    }
};

auto lambda = []()
{
    std::puts("Lambda invoked");
};

int main()
{
    // Declare an argumentum-free signal.
    comp::Signal<void()> signal;

    // Connect the function to the signal.
    signal.connect(&function);

    // Connect the functor to the signal.
    signal.connect(Functor());

    // Connect a method of a shared object.
    auto object = comp::make_shared<Object>();
    signal.connect(object, &Object::method);

    // Connect a lambda.
    signal.connect(lambda);

    // Emit the signal.
    signal();

    return 0;
}
