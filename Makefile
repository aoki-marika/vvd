CC = gcc
CFLAGS = -I/opt/vc/include -Iinclude -Iinclude/vvd
LDFLAGS = -L/opt/vc/lib -lbrcmGLESv2 -lbrcmEGL -lbcm_host -lm -ludev -lportaudio -lsndfile
MKDIR_P = mkdir -p
RM = rm
RM_R = $(RM) -r

SRC = $(wildcard src/*.c)
DEP = $(wildcard include/*.c)
OBJ = $(SRC:.c=.o)

BIN = bin
TARGET = $(BIN)/vvd

$(TARGET): $(OBJ)
	$(MKDIR_P) $(BIN)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) $(OBJ)
	$(RM_R) $(BIN)
