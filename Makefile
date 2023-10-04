NAME			=	Webserv

CXX				=	C++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++98 -MMD

SRCDIR		=	./srcs
INCDIR		=	./inc

SRCS			=	$(SRCDIR)/main.cpp  $(SRCDIR)/Server.cpp

OBJS			=	$(SRCS:.cpp=.o)
DEPS 			:= $(SRCS:.cpp=.d)

$(NAME)		:	$(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o				:	%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

all				:
	@$(MAKE) $(NAME)

-include $(DEPS)

clean			:
	@$(RM) $(OBJS) $(DEPS)

fclean		:	clean
	@$(RM) $(NAME)

re				:
	@make fclean
	@make all

.PHONY		:	all clean fclean re