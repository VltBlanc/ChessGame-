CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -fPIC
SRC = src
OUT = dist
# --- LOGIQUE DÉTECTION OS ---
ifeq ($(OS),Windows_NT)
    # Windows
    LDLIBS = -L/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32
    COMPAT_OBJ = $(OUT)/dlfcn.o
else
    # Mac/Linux
    LDLIBS = -ldl
    COMPAT_OBJ = 
endif


.PHONY: all clean

all: $(OUT)/main $(OUT)/libhuman.so $(OUT)/librandom.so $(OUT)/libminmax.so

$(OUT):
	mkdir -p $(OUT)
	if [ -d "$(SRC)/assets" ]; then cp -r $(SRC)/assets $(OUT)/; fi

# Règle principale utilisant COMPAT_OBJ (sera vide sur Mac, donc ignoré)
$(OUT)/main: $(OUT)/main.o $(OUT)/game.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o $(COMPAT_OBJ) | $(OUT)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OUT)/main.o: $(SRC)/main.c $(SRC)/game.h $(SRC)/chess.h $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

ifeq ($(OS),Windows_NT)
$(OUT)/dlfcn.o: $(SRC)/dlfcn.c $(SRC)/dlfcn.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@
endif

$(OUT)/chess.o: $(SRC)/chess.c $(SRC)/chess.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/game.o: $(SRC)/game.c $(SRC)/game.h $(SRC)/chess.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/util.o: $(SRC)/util.c $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/ds.o: $(SRC)/ds.c $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/human.o: $(SRC)/human.c $(SRC)/chess.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/random.o: $(SRC)/random.c $(SRC)/chess.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/minmax.o: $(SRC)/minmax.c $(SRC)/chess.h $(SRC)/util.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

# Correction des bibliothèques partagées : même logique pour l'ordre
$(OUT)/libhuman.so: $(OUT)/human.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o | $(OUT)
	$(CC) -shared -fPIC $^ -o $@ $(LDLIBS)

$(OUT)/librandom.so: $(OUT)/random.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o | $(OUT)
	$(CC) -shared -fPIC $^ -o $@ $(LDLIBS)

$(OUT)/libminmax.so: $(OUT)/minmax.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o | $(OUT)
	$(CC) -shared -fPIC $^ -o $@ $(LDLIBS)

clean:
	rm -rf $(OUT)
