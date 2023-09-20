#############
# Variables #
#############

INCLUDE_DIRS := $(shell find srcs -type d -print)
CXXFLAGS  = -Wall -Wextra -Werror -pedantic -MMD -MP $(addprefix -I, $(INCLUDE_DIRS))
SRCS      = $(shell find srcs -type f -name "*.cpp")
HEADERS   = $(shell find srcs -type f -name "*.hpp")
OBJDIR    = objs
OBJS      = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))
DEPS      = $(OBJS:.o=.d)
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
	CXXFLAGS += -D LINUX -std=c++98
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

.PHONY: all
all: $(NAME)

.PHONY: clean
clean:
	rm -f $(OBJS) $(DEPS) $(UNIT_OBJS) $(UNIT_DEPS)
	rm -rf $(OBJDIR)

.PHONY: fclean
fclean: clean 
	rm -f $(NAME) $(UNITTEST)

.PHONY: re
re: fclean all

$(OBJS): $(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: debug
debug: CXXFLAGS += -fsanitize=address -fsanitize=undefined -D DEBUG -g
debug: re
d: debug

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

.PHONY: test
test: $(NAME)
	./tests/test.sh

UNIT_SRCS = $(wildcard tests/unit/*.cpp)
UNIT_OBJS = $(UNIT_SRCS:.cpp=.o)
UNIT_DEPS = $(UNIT_SRCS:.cpp=.d)
$(UNITTEST): $(OBJS) $(UNIT_OBJS)
	$(CXX) $(CXXFLAGS) $(UNIT_OBJS) $(filter-out $(OBJDIR)/srcs/main.o, $(OBJS)) -o $(UNITTEST)

.PHONY: unit
unit: $(UNITTEST)
	./$(UNITTEST)

.PHONY: bench
bench: $(NAME)
	./tests/bench.sh

.PHONY: format
format:
	clang-format -style=google $(SRCS) $(HEADERS) -i
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem srcs $(addprefix -I, $(INCLUDE_DIRS))
include $(wildcard $(DEPS))
