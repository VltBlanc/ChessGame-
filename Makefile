CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -fPIC
# Remplacez cette ligne
LDLIBS = -L/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32
# Détection de l'OS
ifeq ($(OS),Windows_NT)
    # Windows : on compile avec la compatibilité dlfcn
    COMPAT_OBJ = $(OUT)/dlfcn.o
else
    # Mac/Linux : pas besoin de dlfcn
    COMPAT_OBJ =
endif
SRC = src
OUT = dist

.PHONY: all clean

all: $(OUT)/main $(OUT)/libhuman.so $(OUT)/librandom.so $(OUT)/libminmax.so

$(OUT):
	mkdir -p $(OUT)
	if [ -d "$(SRC)/assets" ]; then cp -r $(SRC)/assets $(OUT)/; fi

# Correction : placer $(LDLIBS) à la fin de la ligne
# Modifiez la règle du main pour inclure $(COMPAT_OBJ)
$(OUT)/main: $(OUT)/main.o $(OUT)/game.o $(OUT)/chess.o $(OUT)/util.o $(OUT)/ds.o $(COMPAT_OBJ) | $(OUT)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OUT)/main.o: $(SRC)/main.c $(SRC)/game.h $(SRC)/chess.h $(SRC)/ds.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

# Ajoutez cette règle pour ne compiler dlfcn que si nécessaire
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
