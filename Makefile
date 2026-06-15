NAME := webserv
CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -MMD -MP
CPPFLAGS := -Iincludes/config -Iincludes/connection
RM := rm -f

OBJDIR := obj
SRCS := main.cpp \
	srcs/config/ConfigParser.cpp \
	srcs/config/ServerConfig.cpp \
	srcs/config/LocationConfig.cpp \
	srcs/config/serverDirectives.cpp \
	srcs/config/locationDirectives.cpp \
	srcs/connection/HttpRequest.cpp \
	srcs/connection/HttpResponse.cpp \
	srcs/connection/CgiHandler.cpp \
	srcs/connection/Client.cpp \
	srcs/connection/ServerManager.cpp
OBJS := $(SRCS:%.cpp=$(OBJDIR)/%.o)
DEPS := $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) -r $(OBJDIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re

-include $(DEPS)
