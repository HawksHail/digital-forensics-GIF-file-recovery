#Base Directory
BASE_DIR = ${PWD}

#Compiler configuration
CC = gcc
CPP = g++

# Compilation flags/switches
#DEBUG_FLAGS = -DTRACE_WANTED

CC_FLAGS = -c -g -O -gdwarf-3 -Wunused \
           $(DEBUG_FLAGS) \

CPP_FLAGS = -c -g -O2 -gdwarf-3 -Wunused \
           $(DEBUG_FLAGS) \
