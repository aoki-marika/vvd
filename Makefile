CC = gcc
CFLAGS = -I/opt/vc/include -Iinclude -Iinclude/vvd
LDFLAGS = -L/opt/vc/lib -lbrcmGLESv2 -lbrcmEGL -lbcm_host -lm -ludev -lportaudio -lsndfile
MKDIR_P = mkdir -p
CP = cp
RM = rm
RM_R = $(RM) -r

SRC = $(wildcard src/*.c)
DEP = $(wildcard include/*.c)
OBJ = $(SRC:.c=.o)
SHADERS = $(wildcard shaders/*.fs shaders/*.vs)

BIN = bin
TARGET = $(BIN)/vvd
SHADERS_TARGET = $(BIN)/shaders

all: $(TARGET) $(SHADERS_TARGET)

$(TARGET): $(OBJ)
	$(MKDIR_P) $(BIN)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

# todo: would like this to only occur when shaders change
$(SHADERS_TARGET): $(SHADERS)
	$(MKDIR_P) $(@)
	$(CP) $^ $@

.PHONY: clean
clean:
	$(RM) $(OBJ)
	$(RM_R) $(BIN)
