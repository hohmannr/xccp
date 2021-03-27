VERSION = 0.0.1

PROJECT_NAME = xccp - X Console Colorpicker
BIN_NAME = xccp

X11INC = /usr/include/X11
X11LIB = /usr/lib/X11

INCS = -I$(X11INC)
LIBS = -L$(X11LIB) -lX11

CFLAGS = $(INCS)
LDFLAGS = $(LIBS)
CC = c89

INSTALL_DIR = ~/.local/bin

SRC = src/xccp.c

$(BIN_NAME):
	$(CC) $(SRC) -o $(BIN_NAME) $(CFLAGS) $(LDFLAGS)

debug:
	$(CC) -Wall -g $(SRC) -o $(BIN_NAME) $(CFLAGS) $(LDFLAGS)

install: $(BIN_NAME)
	mkdir -p $(INSTALL_DIR)
	cp $(BIN_NAME) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BIN_NAME)

clean:
	rm -rf $(BIN_NAME)
