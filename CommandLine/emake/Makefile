include ../../Config.mk

SHARED_SRC_DIR := ../../shared

PROTO_DIR := $(SHARED_SRC_DIR)/protos/.eobjs

ifeq ($(OS), Darwin)
	OS_LIBS := -lboost_program_options -lpthread -ldl
else ifeq ($(UNIX_BASED), true)
	OS_LIBS := -lboost_program_options -Wl,--no-as-needed -Wl,-rpath,./ -lpthread -ldl
else
	OS_LIBS := -lboost_system-mt -Wl,--no-as-needed -Wl,-rpath,./ -lboost_program_options-mt -lpthread
endif

ifeq ($(UNIX_BASED), true)
	ifeq ($(OS), Darwin)
		OS_LIBS += -framework CoreFoundation -framework CoreGraphics -framework Cocoa
	endif
endif

CXXFLAGS  += -I../../CompilerSource -I$(PROTO_DIR) -I../$(LIB_PFX)EGM -I../$(LIB_PFX)EGM
LDFLAGS   += $(OS_LIBS) -L../../ -lcompileEGMf$(SUFFIX) -lEGM$(SUFFIX) -lProtocols$(SUFFIX) -lENIGMAShared$(SUFFIX) -lgrpc++ -lprotobuf -lyaml-cpp -lpng
ifeq ($(OS), FreeBSD)
	LDFLAGS += $(shell pkg-config --libs --cflags protobuf) -Wl,--allow-shlib-undefined
else ifeq ($(OS), DragonFly)
	LDFLAGS += $(shell pkg-config --libs --cflags protobuf) -Wl,--allow-shlib-undefined
else ifeq ($(OS), OpenBSD)
	LDFLAGS += $(shell pkg-config --libs --cflags protobuf) -Wl,--allow-shlib-undefined
endif

ifeq ($(TESTS), TRUE)
	TARGET=../../emake-tests$(SUFFIX)
	SOURCES := $(call rwildcard, ../emake-tests,*.cpp)
	LDFLAGS += -lpthread -lgtest_main -lgtest
else
	TARGET = ../../emake$(SUFFIX)
	SOURCES := $(call rwildcard,$(SRC_DIR),*.cpp)
endif

ifeq ($(OS), Darwin)
	LINKING_TARGETS += make AppleFixLinks
endif

AppleFixLinks:
	install_name_tool -change ../../$(LIB_PFX)Protocols$(LIB_EXT) @loader_path/./$(LIB_PFX)Protocols$(LIB_EXT) ../../$(LIB_PFX)EGM$(LIB_EXT)
	install_name_tool -change ../$(LIB_PFX)ENIGMAShared$(LIB_EXT) @loader_path/./$(LIB_PFX)ENIGMAShared$(LIB_EXT) ../../$(LIB_PFX)EGM$(LIB_EXT)
	install_name_tool -change ../../$(LIB_PFX)Protocols$(LIB_EXT) @loader_path/./$(LIB_PFX)Protocols$(LIB_EXT) ../../$(LIB_PFX)compileEGMf.dylib
	install_name_tool -change ../$(LIB_PFX)ENIGMAShared$(LIB_EXT) @loader_path/./$(LIB_PFX)ENIGMAShared$(LIB_EXT) ../../$(LIB_PFX)compileEGMf.dylib
	install_name_tool -change ../../$(LIB_PFX)EGM$(LIB_EXT) @loader_path/./$(LIB_PFX)EGM$(LIB_EXT) $(TARGET)
	install_name_tool -change ../$(LIB_PFX)compileEGMf$(LIB_EXT) @loader_path/./$(LIB_PFX)compileEGMf.dylib $(TARGET)
	install_name_tool -change ../$(LIB_PFX)compileEGMf.dylib @loader_path/./$(LIB_PFX)compileEGMf.dylib $(TARGET)
	install_name_tool -change ../$(LIB_PFX)ENIGMAShared$(LIB_EXT) @loader_path/./$(LIB_PFX)ENIGMAShared$(LIB_EXT) $(TARGET)
	install_name_tool -change ../../$(LIB_PFX)Protocols$(LIB_EXT) @loader_path/./$(LIB_PFX)Protocols$(LIB_EXT) $(TARGET)

include ../../Default.mk
