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

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $(NAME)

test: $(NAME)
	./tests/test.sh

format:
	clang-format -style=google $(SRCS) $(INCLUDES) -i
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem srcs -I $(INCLUDES)
include $(wildcard $(DEPS))
