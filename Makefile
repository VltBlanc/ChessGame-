CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -fPIC
SRC = src
OUT = dist



# --- LOGIQUE DÉTECTION OS ---
ifeq ($(OS),Windows_NT)
    # Windows
    LDLIBS = -L/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        # macOS
        # On suppose que raylib est installé via Homebrew (/opt/homebrew)
        LDLIBS = -L/opt/homebrew/lib -lraylib -ldl -lpthread -lm \
                 -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
    else
        # Linux
        LDLIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
endif


.PHONY: all clean

all: $(OUT)/main

$(OUT):
	mkdir -p $(OUT)
	if [ -d "$(SRC)/assets" ]; then cp -r $(SRC)/assets $(OUT)/; fi

$(OUT)/main: $(OUT)/main.o $(OUT)/game.o $(OUT)/human.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o | $(OUT)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OUT)/main.o: $(SRC)/main.c $(SRC)/game.h $(SRC)/chess.h $(SRC)/human.h $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/chess.o: $(SRC)/chess.c $(SRC)/chess.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/game.o: $(SRC)/game.c $(SRC)/game.h $(SRC)/chess.h $(SRC)/human.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/util.o: $(SRC)/util.c $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/ds.o: $(SRC)/ds.c $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/human.o: $(SRC)/human.c $(SRC)/human.h $(SRC)/chess.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OUT)

run :
	./dist/main game
