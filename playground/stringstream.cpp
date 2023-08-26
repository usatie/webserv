#include <vector>
#include <iostream>
#include <string>
#include <sstream>

int main()
{
    std::cout << "Hello, world!\n";
    std::stringstream ss;
    ss.exceptions(std::ios::badbit | std::ios::failbit);
    try {
        std::string hugeString(1000, 'A');
        for (;;)
        {
            std::cout << ".";
            ss << hugeString;
            if (ss.bad() || ss.fail()) {
                std::cout << std::endl;
                std::cout << ss.bad() << ", " << ss.fail() << std::endl;
                break;
            }
        }
    }
    catch (std::bad_alloc& e) {
        // Memory allocation failed.
        std::cout << "Memory allocation failed: " << e.what() << '\n';
                // Release the allocated memory before exiting the loop.
    }
    std::cout << "Goodbye, world!\n";
    return 0;
}

