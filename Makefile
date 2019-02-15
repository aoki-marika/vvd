CC = gcc
CFLAGS = -I/opt/vc/include -Iinclude -Iinclude/vvd
BIN = bin
LDFLAGS = -L/opt/vc/lib -L$(BIN) -lbrcmGLESv2 -lbrcmEGL -lbcm_host -lm -ludev -lbass -Wl,-rpath,"\$$ORIGIN"
MKDIR_P = mkdir -p
CP = cp
RM = rm
RM_R = $(RM) -r

SRC = $(wildcard src/*.c)
DEP = $(wildcard include/*.c)
OBJ = $(SRC:.c=.o)
SHADERS = $(wildcard shaders/*.fs shaders/*.vs)

TARGET = $(BIN)/vvd
SHADERS_TARGET = $(BIN)/shaders
BASS_TARGET = $(BIN)/libbass.so

all: $(BASS_TARGET) $(TARGET) $(SHADERS_TARGET)

$(TARGET): $(OBJ)
	$(MKDIR_P) $(BIN)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

# todo: would like this to only occur when shaders change
$(SHADERS_TARGET): $(SHADERS)
	$(MKDIR_P) $(@)
	$(CP) $^ $@

$(BASS_TARGET):
	$(CP) lib/libbass.so $@

.PHONY: clean
clean:
	$(RM) $(OBJ)
	$(RM_R) $(BIN)
