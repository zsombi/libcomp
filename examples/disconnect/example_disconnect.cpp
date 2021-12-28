#include <comp/signal>
#include <iostream>

void function()
{
}

auto lambda = [](comp::ConnectionPtr connection)
{
    connection->disconnect();
};

int main()
{
    // Declare an argumentum-free signal.
    comp::Signal<void()> signal;

    // Connect the function to the signal.
    auto connection = signal.connect(&function);

    // Connect an auto-disconnectiong lambda.
    signal.connect(lambda);

    // Emit the signal.
    std::cout << "Emit count: " << signal() << std::endl;
    std::cout << "Emit count now: " << signal() << std::endl;

    return 0;
}
