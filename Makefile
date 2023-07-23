CXXFLAGS  = -std=c++98 -Wall -Wextra -Werror -pedantic
SRCS      = $(wildcard srcs/*.cpp)
INCLUDES  = $(wildcard srcs/*.hpp)
NAME      = webserv

$(NAME): $(SRCS)
	$(CXX) $(CXXFLAGS) -I $(INCLUDES) $(SRCS) -o $(NAME)

test: $(NAME)
	./tests/test.sh

format:
	clang-format -style=google $(SRCS) $(INCLUDES) -i
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem srcs -I $(INCLUDES)
