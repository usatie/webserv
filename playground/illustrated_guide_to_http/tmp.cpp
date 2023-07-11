#include <iostream>
#include <sstream>
#include <fstream>
 
int main()
{
    char x[256];
    //std::istringstream stream("Hello\nWorld\r42Tokyo\r\n");
    std::ifstream stream("hethmon_src/Tester/input.txt");
    if (!stream) {
        std::cout << "Could not open file\n";
        return 1;
    }
 
    while (!stream.eof()) {
      stream.getline(x, sizeof x, '\n');
      std::cout << "strlen(x): " << strlen(x) << std::endl;
      std::cout << "Characters extracted: " << stream.gcount() << std::endl;
    }
}
