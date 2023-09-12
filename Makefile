#############
# Variables #
#############

INCLUDES  = $(wildcard include/*.hpp)
CXXFLAGS  = -Wall -Wextra -Werror -pedantic -MMD -MP -I include -I srcs
SRCS      = $(wildcard srcs/*.cpp)
OBJS	  = $(SRCS:.cpp=.o)
DEPS	  = $(SRCS:.cpp=.d)
NAME      = webserv
UNITTEST  = unit_test

##########################
# Platform Compatibility #
##########################

# LINUX | OSX | ARM
UNAME_S := $(shell uname -s)
UNAME_P := $(shell uname -p)

# Linux 
# TODO: Replace C++11 with C++98
# Currently, we're using some C++11 features like shared_ptr.
# We need to replace them with C++98 features.
# (clang on macos doesn't throw any error, but g++ on linux does.)
ifeq ($(UNAME_S),Linux)
	CXXFLAGS += -D LINUX -std=c++11
endif

# macos x86
ifeq ($(UNAME_S),Darwin)
	CXXFLAGS += -D OSX -std=c++98
endif

# macos ARM (m1/m2...)
ifneq ($(filter arm%, $(UNAME_P)),)
	CXXFLAGS += -D ARM -std=c++98
endif

#################
# General rules #
#################

all: $(NAME)

clean:
	rm -f $(OBJS) $(DEPS) $(UNIT_OBJS) $(UNIT_DEPS)

fclean: clean 
	rm -f $(NAME) $(UNITTEST)

re: fclean all

%.o: %.cpp %.d
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: CXXFLAGS = -std=c++98 -Wall -Wextra -pedantic -MMD -MP -fsanitize=address -fsanitize=undefined -D DEBUG -I include -I srcs
debug: re
d: debug

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

test: $(NAME)
	./tests/test.sh

UNIT_SRCS = $(wildcard tests/unit/*.cpp)
UNIT_OBJS = $(UNIT_SRCS:.cpp=.o)
UNIT_DEPS = $(UNIT_SRCS:.cpp=.d)
$(UNITTEST): $(OBJS) $(UNIT_OBJS)
	$(CXX) -I srcs -I include $(UNIT_OBJS) $(filter-out srcs/main.o, $(OBJS)) -o $(UNITTEST)

.PHONE: unit
unit: $(UNITTEST)
	./$(UNITTEST)


format:
	clang-format -style=google $(SRCS) $(INCLUDES) -i
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem srcs -I $(INCLUDES)
include $(wildcard $(DEPS))
