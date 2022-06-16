CC=g++
CXX=g++
RANLIB=ranlib

LIBSRC=container.cpp sockets.cpp
LIBOBJ=$(LIBSRC:.cpp=.o)


INCS=-I.
CFLAGS = -Wall -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

CONTAINER = container
SOCKET = sockets
TARGETS = $(CONTAINER) $(SOCKET)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar
TARSRCS=$(LIBSRC) Makefile README

all: $(TARGETS)

container: container.cpp
	$(CC) $(CFLAGS) container.cpp -o container
sockets: sockets.cpp
	$(CC) $(CFLAGS) sockets.cpp -o sockets
#$(CONTAINER):$(LIBOBJ)
#	$(AR) $(ARFLAGS) $@ $^
#	$(RANLIB) $@
#
#$(SOCKET):$(LIBOBJ)
#	$(AR) $(ARFLAGS) $@ $^
#	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(UTHREADLIB) $(OBJ) $(LIBOBJ) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(SRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)