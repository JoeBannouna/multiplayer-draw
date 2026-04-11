CC      := clang
# CFLAGS  := -std=gnu11 -Wall -Wextra -Werror -O2 -pthread $(shell pkg-config --cflags sdl2) -fsanitize=address,undefined
CFLAGS  := -std=gnu11 -Wall -Wextra -Werror -O2 -pthread -Wconversion
LDFLAGS := $(shell pkg-config --libs sdl2)

CLIENT      := client/main.c
CLIENT_DEPS := $(wildcard client/*.c client/*.h)
CLIENT_BIN  := build/client

SERVER      := server/app.c
SERVER_DEPS := $(wildcard server/*.c server/*.h)
SERVER_BIN  := build/server

CORE_DEPS   := $(wildcard core/*.c core/*.h)

client: $(CLIENT_BIN)

server: $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT) $(CLIENT_DEPS) $(CORE_DEPS)
	$(CC) $(CFLAGS) $(CLIENT) -o $(CLIENT_BIN) $(LDFLAGS) $(shell pkg-config --cflags sdl2)

$(SERVER_BIN): $(SERVER) $(SERVER_DEPS) $(CORE_DEPS)
	$(CC) $(CFLAGS) $(SERVER) -o $(SERVER_BIN)

run: $(CLIENT_BIN)
	./$(CLIENT_BIN)

clean:
	rm -f $(BIN)
