NAME= webserv
CXX= c++
CXXFLAGS= -Wall -Wextra -Werror -std=c++98
RM= rm -f

OBJDIR= obj

OBJS= $(OBJDIR)/main.o \
		$(OBJDIR)/ConfigParser.o \
		$(OBJDIR)/ServerConfig.o \
		$(OBJDIR)/LocationConfig.o \
		$(OBJDIR)/serverDirectives.o \
		$(OBJDIR)/locationDirectives.o

all: $(OBJDIR) $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/ConfigParser.o: srcs/ConfigParser.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/ServerConfig.o: srcs/ServerConfig.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/LocationConfig.o: srcs/LocationConfig.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/serverDirectives.o: srcs/serverDirectives.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/locationDirectives.o: srcs/locationDirectives.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	$(RM) -r $(OBJDIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
