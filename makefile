UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	FLAGS = -lglut -lGL -lGLU -lGLEW -std=c++11 -lSOIL
endif
ifeq ($(UNAME_S),Darwin)
	FLAGS = -framework GLUT -framework OpenGL -std=c++11 -lSOIL
endif

# OBJS specifies which files to compile as part of the project
OBJS = src/doll.cc

# CC specifies which compiler we're using
CC = g++

# OBJ_NAME specifies the name of our exectuable
OBJ_NAME = runThis

#This is the target that compiles our executable
.PHONY: dots
dots :
	$(CC) $(OBJS) $(FLAGS) -o $(OBJ_NAME)
