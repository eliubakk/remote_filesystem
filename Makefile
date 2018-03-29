CXX = g++
CXXFLAGS = -g -std=c++11 -Wall -Werror
DEBUGFLAGS = -DDEBUG

SERVER_LIBS = libfs_server.o -pthread -ldl
CLIENT_LIBS = libfs_client.o -ldl

EXECUTABLE = fs

PROJECTFILE = fs.cc

TESTSOURCES = $(wildcard test*.cc)
TESTS = $(TESTSOURCES:%.cc=%)

SOURCES = $(wildcard *.cc)
SOURCES := $(filter-out $(TESTSOURCES), $(SOURCES))
OBJECTS = $(SOURCES:%.cc=%.o)

all: $(EXECUTABLE) alltests
	./createfs
	export FS_CRYPT=AES

release: CXXFLAGS += -DNDEBUG
release: all

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: clean all

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(SERVER_LIBS) -o $(EXECUTABLE)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $*.cc

define make_tests
    SRCS = $$(filter-out $$(PROJECTFILE), $$(SOURCES))
    OBJS = $$(SRCS:%.cc=%.o)
    HDRS = $$(wildcard *.h)
    $(1): CXXFLAGS += $$(DEBUGFLAGS)
    $(1): $$(HDRS) $$(OBJS) $(1).cc
	$$(CXX) $$(CXXFLAGS) $$(CLIENT_LIBS) $(1).cc -o $(1)
endef
$(foreach test, $(TESTS), $(eval $(call make_tests, $(test))))

alltests: $(TESTS)

clean: 
	rm -f $(OBJECTS) $(EXECUTABLE) $(TESTS)

.PHONY: all clean alltests debug release
.SUFFIXES: