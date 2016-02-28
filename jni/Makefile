ARCH:=$(shell uname -m)
BIT32:=i686
BIT64:=x86_64
OBJ=                   \
	AHandler.o         \
	ALooper.o          \
	ALooperRoster.o    \
	AMessage.o         \
	AString.o          \
	ABuffer.o          \
	handler_1.o        \
	base64.o           \
	our_md5.o          \
	AAtomizer.o        \
	ARTSPConnection.o  \
	MyRTSPHandler.o    \
	stage_utils.o      \
	ARTPSource.o       \
	ARTPConnection.o

CC=g++
CFLAGS=-Wno-multichar

ifeq ($(ARCH),$(BIT64))
FLAG64=-fPIC
endif

INCLUDES= -I./include/
LDLIBS= -L. -lpthread -luuid
test:test.cpp libstagefright_foundation.so
	$(CC)  $(FLAG64) $(CFLAGS) $(INCLUDES) test.cpp  libstagefright_foundation.so -lpthread -o test
libstagefright_foundation.so:$(OBJ)
	$(CC) -shared  $(FLAG64)  $(OBJ) $(LDLIBS) -o libstagefright_foundation.so 	
$(OBJ): %.o: %.cpp
	$(CC) -c $(FLAG64) $(CFLAGS) $(INCLUDES)  $< -o $@
.PHONY:clean
clean:
	rm -f *.o libstagefright_foundation.so
