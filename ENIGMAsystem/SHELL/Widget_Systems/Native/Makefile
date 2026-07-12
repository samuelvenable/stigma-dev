LIBDLGMOD := $(shell chmod u+x Widget_Systems/Native/libdlgmod/build.sh)
LIBDLGMOD += $(shell Widget_Systems/Native/libdlgmod/build.sh 2> /dev/null)
SOURCES += Widget_Systems/Native/dialogs.cpp
override LDFLAGS += Widget_Systems/Native/libdlgmod/libdlgmod.a
override LDLIBS += Widget_Systems/Native/libdlgmod/libdlgmod.a
ifeq ($(UNIX_BASED), true)
	ifeq ($(OS), Darwin)
		override LDFLAGS += -framework AppKit -framework UniformTypeIdentifiers 
		override LDLIBS += -framework AppKit -framework UniformTypeIdentifiers
	else
		ifeq ($(OS), Linux)
			override LDFLAGS += -lX11 -lpthread
			override LDLIBS += -lX11 -lpthread
		else
			override LDFLAGS += -lX11 -lpthread -lc -lkvm
			override LDLIBS += -lX11 -lpthread -lc -lkvm
		endif
	endif
else
	override LDFLAGS += -lntdll -lgdiplus -lcomctl32 -lshlwapi -lcomdlg32 -lole32 -loleaut32 -luuid
	override LDLIBS += -lntdll -lgdiplus -lcomctl32 -lshlwapi -lcomdlg32 -lole32 -loleaut32 -luuid
endif
