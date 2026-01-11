EE_BIN  = ps2_live_graph_studio.elf
EE_OBJS = \
  src/main.o \
  src/runtime/runtime.o \
  src/system/pad.o \
  src/system/timing.o \
  src/render/render.o \
  src/render/font.o \
  src/graph/graph_core.o \
  src/graph/graph_validate.o \
  src/graph/graph_eval.o \
  src/graph/graph_publish.o \
  src/nodes/node_registry.o \
  src/nodes/node_basic.o \
  src/nodes/node_extended.o \
  src/io/graph_io.o \
  src/io/assets.o \
  src/io/assets_embedded_data.o \
  src/ui/editor.o \
  src/ui/command_palette.o

EE_CFLAGS = -O2 -Wall -Wextra -std=c99 -ffast-math
EE_LDFLAGS = -L/usr/local/ps2dev/gsKit/lib
EE_INCS = -I/usr/local/ps2dev/gsKit/include
EE_LIBS = -lpad -ldebug -lc -lgs -ldma -lgraph -lgskit -lgskit_toolkit -ldmakit

# If your gsKit install uses different libs, adjust EE_LIBS accordingly.
# Some setups use: -lgskit -lgskit_toolkit -ldmakit

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal