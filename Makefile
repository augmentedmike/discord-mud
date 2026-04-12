OS := $(shell uname -s)

ifeq ($(OS),Darwin)
include Makefile.mac
else ifeq ($(OS),Linux)
include Makefile.linux
else
$(error Unsupported OS: $(OS))
endif