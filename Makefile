CXXFLAGS  = -std=c++98 -Wall -Wextra -Werror -pedantic
SRCS      = $(wildcard srcs/*.cpp)
INCLUDES  = -I srcs
NAME      = webserv

$(NAME): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $(NAME)

test: $(NAME)
	./tests/test.sh
