CXXFLAGS  = -std=c++98 -Wall -Wextra -Werror -pedantic
SRCS      = $(wildcard srcs/*.cpp)
NAME      = webserv

$(NAME): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(NAME)

test: $(NAME)
	./tests/test.sh
