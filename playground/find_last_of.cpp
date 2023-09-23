#include <iostream>
#include <string>

int main() {
	{
    std::cout << "Test1: " << std::endl;
	  std::string script_path = "/home/username/script.sh";
	  std::string dir_path = script_path.substr(0, script_path.find_last_of('/'));
    std::string script_name = script_path.substr(script_path.find_last_of('/') + 1);
    std::cout << "script_path: " << script_path << std::endl;
    std::cout << "dir_path   : " << dir_path << std::endl;
    std::cout << "script_name: " << script_name << std::endl;
	}
	{
    std::cout << "Test2: " << std::endl;
	  std::string script_path = "/home/username/script/";
	  std::string dir_path = script_path.substr(0, script_path.find_last_of('/'));
    std::string script_name = script_path.substr(script_path.find_last_of('/') + 1);
    std::cout << "script_path: " << script_path << std::endl;
    std::cout << "dir_path   : " << dir_path << std::endl;
    std::cout << "script_name: " << script_name << std::endl;
	}
  {
    std::cout << "Test3: " << std::endl;
	  std::string script_path = "script.sh";
	  std::string dir_path = script_path.substr(0, script_path.find_last_of('/'));
    std::string script_name = script_path.substr(script_path.find_last_of('/') + 1);
    std::cout << "script_path: " << script_path << std::endl;
    std::cout << "dir_path   : " << dir_path << std::endl;
    std::cout << "script_name: " << script_name << std::endl;
  }
}
