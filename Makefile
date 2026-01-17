NAME = ircserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP -Iheaders

# ----------------------------------------------------------
# Sources
# ----------------------------------------------------------

CLASSFILES =	classes/Client.cpp \
				classes/CommandHandler.cpp \
				classes/Server.cpp \
				classes/Parser.cpp \
				classes/Channel.cpp \
				classes/ServerResponse.cpp \

SRC = main.cpp $(CLASSFILES)

# ----------------------------------------------------------
# Objects & Dependencies
# ----------------------------------------------------------

OBJ_DIR = obj
OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))
DEP = $(OBJ:.o=.d)

# ----------------------------------------------------------
# Build rules
# ----------------------------------------------------------

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

# Create object directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Pattern rule for building object files
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ----------------------------------------------------------
# Cleanup rules
# ----------------------------------------------------------

clean:
	$(RM) -r $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re
