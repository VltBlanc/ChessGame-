CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -fPIC
# Remplacez cette ligne
LDLIBS = -L/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32
SRC = src
OUT = dist

.PHONY: all clean

all: $(OUT)/main $(OUT)/libhuman.so $(OUT)/librandom.so $(OUT)/libminmax.so

$(OUT):
	mkdir -p $(OUT)
	if [ -d "$(SRC)/assets" ]; then cp -r $(SRC)/assets $(OUT)/; fi

# Correction : placer $(LDLIBS) à la fin de la ligne
$(OUT)/main: $(OUT)/main.o $(OUT)/game.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o $(OUT)/dlfcn.o | $(OUT)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OUT)/main.o: $(SRC)/main.c $(SRC)/game.h $(SRC)/chess.h $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/dlfcn.o: src/dlfcn.c src/dlfcn.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

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
