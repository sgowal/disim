include ../Makefile.include

MAIN_SOURCE = display/SimViewer.cpp
SOURCES = cmdline.c
ifeq ($(GUI), 1)
CPP_SOURCES = $(MAIN_SOURCE) utils/Fl_Glv_Window.cpp  utils/Log.cpp \
              engine/Simulator.cpp agents/Car.cpp agents/CarState.cpp \
              display/TextureManager.cpp display/RealisticDrawer.cpp \
              agents/CarControl.cpp map/Map.cpp display/Model_3DS.cpp \
              display/LaneOptions.cpp
else
CPP_SOURCES = $(MAIN_SOURCE) utils/Log.cpp \
              engine/Simulator.cpp agents/Car.cpp agents/CarState.cpp \
              agents/CarControl.cpp map/Map.cpp 
endif
ifeq ($(LUA), 1)
CPP_SOURCES += bindings/lua/LuaBinding.cpp bindings/lua/LuaLane.cpp \
               bindings/lua/LuaCar.cpp bindings/lua/LuaRoadSensor.cpp \
               bindings/lua/LuaInfrastructure.cpp bindings/lua/LuaRoadActuator.cpp
endif

ifndef OSTYPE
  OSTYPE = $(shell uname -s|awk '{print tolower($$0)}')
endif

CC = g++ -m$(ARCH)
CFLAGS = -O3 -Wall -I. -fno-strict-aliasing -Wno-write-strings
ifeq ($(GUI), 1)
CFLAGS += -DGUI
endif
ifeq ($(LUA), 1)
CFLAGS += -DLUA
endif
ifeq ($(OSTYPE), darwin)
  CFLAGS += -DMAC
else
  CFLAGS += -Wno-unused-result
endif

LIBS = -lpng -lpthread -lm
LIBRARIES =
ifeq ($(GUI), 1)
  ifneq ($(OSTYPE), darwin)
    LIBS += -lGL -lGLU -lglut
  else
    LIBS += -framework GLUT -framework OpenGL -framework Carbon
  endif
  ifeq ($(FLTK_PRESENT), 1)
    LIBS += -lfltk -lfltk_gl
    ifeq ($(OSTYPE), darwin)
      LIBS += -L/sw/lib/fltk-aqua/lib/
    endif
  else
    LIBRARIES += ./libraries/libfltk$(ARCH).a ./libraries/libfltk$(ARCH)_gl.a
    CFLAGS += -I./libraries
  endif
endif

ifeq ($(LUA), 1)
  ifneq ($(OSTYPE), darwin)
    ifeq ($(LUA_PRESENT), 1)
      LIBS += -llua5.1
      CFLAGS += -I/usr/include/lua5.1
    else
      LIBRARIES += ./libraries/liblua$(ARCH).a
      LIBS += -ldl
      CFLAGS += -I./libraries/LUA
    endif
  else
    LIBS += -L/sw/lib/ -llua
    CFLAGS += -I/sw/include/
  endif
endif

ifeq ($(GENGETOPT_PRESENT), 1)
  GGO = gengetopt
else
  GGO = ./external/gengetopt$(ARCH)
endif

OBJECTS = $(SOURCES:.c=.o) $(CPP_SOURCES:.cpp=.o)
LDFLAGS = $(LIBS)

all: $(SOURCES) $(CPP_SOURCES) disim

cmdline.c: cmdline.ggo
	$(GGO) --input="cmdline.ggo"

disim:  $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBRARIES) -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJECTS) disim disim.o cmdline.c cmdline.h
