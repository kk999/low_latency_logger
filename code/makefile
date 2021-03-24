# CC:=g++
CC:=clang++
# CFLAGS:=-Wall -ansi -std=c++11 -static -g
# CFLAGS:=-Wall -ansi -std=c++17 -static -O2 -DNDEBUG
CFLAGS:=-Wall -ansi -std=c++17 -static -O2 -mcmodel=large
# CFLAGS:= -g -pthread -O1 -Wall -DDEBUG -DUDEBUG -D_VR_LINUX -DHAVE_SYS_RESOURCE_H -DHAVE_STRUCT_RLIMIT -std=c++17
# CFLAGS:= -g -pthread -Wall -DDEBUG -DUDEBUG -D_VR_LINUX -DHAVE_SYS_RESOURCE_H -DHAVE_STRUCT_RLIMIT -std=c++17
srcdir:=.
objdir:=../obj
# obj:=main.o $(objdir)/TTUtility.o
obj:=main.o
exe:=../bin/go

all:$(objdir)/$(obj)
	$(CC) -lpthread -o $(exe) $^
$(objdir)/%.o:$(srcdir)/%.cpp
	printf "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
	$(CC) $(CFLAGS) -c $^ -o $@

# $(objdir)/TTUtility.o:../../../../lib/TTUtility.cpp
# 	$(CC) $(CFLAGS) -c $^ -o $(objdir)/$@

.PHONY:clean
clean:
	rm -rf $(objdir)/$(obj) $(exe)

go:
	$(exe) 100 a b

install:$(exe)
	scp $(exe) tradetron@omstest:/home/tradetron/libs/libsrc/ut/go.new

release:
	scp Logger.hpp nplog.h nqlog.h makefile main.cpp jimmy@omstest:/home/jimmy/Logger/core/
