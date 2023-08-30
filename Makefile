INCLUDES  = $(wildcard srcs/*.hpp)
CXXFLAGS  = -std=c++98 -Wall -Wextra -Werror -pedantic -MMD -MP -I include -I srcs
SRCS      = $(wildcard srcs/*.cpp)
OBJS	  = $(SRCS:.cpp=.o)
DEPS	  = $(SRCS:.cpp=.d)
NAME      = webserv
UNITTEST  = unit

all: $(NAME)

clean:
	rm -f $(OBJS) $(DEPS) $(UNIT_OBJS) $(UNIT_DEPS)

fclean: clean 
	rm -f $(NAME) $(UNITTEST)

re: fclean all

# std::shared_ptr is not supported in c++98 so g++ cannot compile it but clang can compile,
# so we need to use c++11 on Linux
linux: CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -MMD -MP
linux: re

%.o: %.cpp %.d
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: CXXFLAGS = -std=c++98 -Wall -Wextra -pedantic -MMD -MP -fsanitize=address -fsanitize=undefined -D DEBUG
debug: re
d: debug

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

test: $(NAME)
	./tests/test.sh

UNIT_SRCS = $(wildcard tests/unit/*.cpp)
UNIT_OBJS = $(UNIT_SRCS:.cpp=.o)
UNIT_DEPS = $(UNIT_SRCS:.cpp=.d)
unit: $(OBJS) $(UNIT_OBJS)
	$(CXX) -I srcs -I include $(UNIT_OBJS) $(filter-out srcs/main.o, $(OBJS)) -o $(UNITTEST)
	./$(UNITTEST)

format:
	clang-format -style=google $(SRCS) $(INCLUDES) -i
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem srcs -I $(INCLUDES)
include $(wildcard $(DEPS))
