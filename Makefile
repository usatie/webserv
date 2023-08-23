CXXFLAGS  = -std=c++98 -Wall -Wextra -Werror -pedantic -MMD -MP
SRCS      = $(wildcard srcs/*.cpp)
OBJS	  = $(SRCS:.cpp=.o)
DEPS	  = $(SRCS:.cpp=.d)
INCLUDES  = $(wildcard srcs/*.hpp)
NAME      = webserv

all: $(NAME)

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean 
	rm -f $(NAME)

re: fclean all

# std::shared_ptr is not supported in c++98 so g++ cannot compile it but clang can compile,
# so we need to use c++11 on Linux
linux: CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -MMD -MP
linux: re

debug: CXXFLAGS = -std=c++98 -Wall -Wextra -pedantic -MMD -MP -fsanitize=address -fsanitize=undefined -D DEBUG
debug: re
d: debug

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

test: $(NAME)
	./tests/test.sh

format:
	clang-format -style=google $(SRCS) $(INCLUDES) -i
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem srcs -I $(INCLUDES)
include $(wildcard $(DEPS))
