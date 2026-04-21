#include "game.h"
#include "../reseau/protocol.h"
#include "chess.h"
#include "human.h"
#include "util.h"
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>


extern DS_ALLOCATOR allocator;

static chess_state_t state = {0};
static int checkmate_gui = 0;
static int stalemate_gui = 0;
static int draw_gui = 0;
static int is_in_check = 0;

static ds_dynamic_array moves = {0}; /* move_t */

static void chess_print_board() {
  int cell_width = SCREEN_WIDTH / CHESS_WIDTH;
  int cell_height = SCREEN_HEIGHT / CHESS_HEIGHT;

  square_t king_square = {0};
  for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
    for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
      int is_light_square = (file + rank) % 2 == 1;
      int square_color = is_light_square ? SQUARE_LIGHT : SQUARE_DARK;
      Color color = {.r = (square_color & 0xFF0000) >> 16,
                     .g = (square_color & 0x00FF00) >> 8,
                     .b = (square_color & 0x0000FF) >> 0,
                     .a = 0xFF};

      int file_px = file * cell_width;
      int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;

      DrawRectangle(file_px, rank_px, cell_width, cell_height, color);

      square_t square = (square_t){.rank = rank, .file = file};
      char piece = chess_square_get(&state.board, square);
      char piece_color = piece & COLOR_FLAG;
      char piece_type = piece & PIECE_FLAG;
      if (piece_color == state.current_player && piece_type == CHESS_KING) {
        king_square = square;
      }
    }
  }

  if (is_in_check) {
    int file = king_square.file;
    int rank = king_square.rank;

    int square_color = SQUARE_MOVE;
    Color color = {.r = (square_color & 0xFF0000) >> 16,
                   .g = (square_color & 0x00FF00) >> 8,
                   .b = (square_color & 0x0000FF) >> 0,
                   .a = 0xFF};

    int file_px = file * cell_width;
    int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;

    DrawRectangle(file_px, rank_px, cell_width, cell_height, color);
  }

  if (state.last_move) {
    int file = state.last_move_start.file;
    int rank = state.last_move_start.rank;

    int file_px = file * cell_width;
    int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;
    int is_light_square = (file + rank) % 2 == 1;
    int square_color = is_light_square ? SQUARE_LIGHT_MOVE : SQUARE_DARK_MOVE;
    Color color = {.r = (square_color & 0xFF0000) >> 16,
                   .g = (square_color & 0x00FF00) >> 8,
                   .b = (square_color & 0x0000FF) >> 0,
                   .a = 0xFF};

    DrawRectangle(file_px, rank_px, cell_width, cell_height, color);
  }

  if (state.last_move) {
    int file = state.last_move_end.file;
    int rank = state.last_move_end.rank;

    int file_px = file * cell_width;
    int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;
    int is_light_square = (file + rank) % 2 == 1;
    int square_color = is_light_square ? SQUARE_LIGHT_MOVE : SQUARE_DARK_MOVE;
    Color color = {.r = (square_color & 0xFF0000) >> 16,
                   .g = (square_color & 0x00FF00) >> 8,
                   .b = (square_color & 0x0000FF) >> 0,
                   .a = 0xFF};

    DrawRectangle(file_px, rank_px, cell_width, cell_height, color);
  }

  for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
    for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
      int file_px = file * cell_width;
      int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;

      square_t square = (square_t){.rank = rank, .file = file};
      char piece = chess_square_get(&state.board, square);
      if (piece != CHESS_NONE) {
        Texture2D texture = LoadTextureCachedPiece(piece);
        float scale = (float)cell_width / texture.width;
        DrawTextureEx(texture, (Vector2){.x = file_px, .y = rank_px}, 0, scale,
                      WHITE);
      }
    }
  }
}

static void chess_print_state() {
  char *fen = NULL;
  chess_dump_fen(&state, &fen);
  DS_LOG_INFO("FEN: %s", fen);
}

static void chess_print_message(const char *text) {
  int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2;
  int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

  Color color = {.r = (0xFF0000 & 0xFF0000) >> 16,
                 .g = (0xFF0000 & 0x00FF00) >> 8,
                 .b = (0xFF0000 & 0x0000FF) >> 0,
                 .a = 0xFF};

  DrawRectangle(col_px, row_px, PROMOTION_GUI_WIDTH, PROMOTION_GUI_HEIGHT,
                color);

  int text_width = MeasureText(text, 20);
  int text_height = 20;
  int text_x = col_px + PROMOTION_GUI_WIDTH / 2 - text_width / 2;
  int text_y = row_px + PROMOTION_GUI_HEIGHT / 2 - text_height / 2 - 10;

  DrawText(text, text_x, text_y, 20, WHITE);
  
  const char *subtext = "Appuyez sur ESPACE pour quitter";
  int subtext_width = MeasureText(subtext, 15);
  int subtext_x = col_px + PROMOTION_GUI_WIDTH / 2 - subtext_width / 2;
  int subtext_y = text_y + 25;
  
  DrawText(subtext, subtext_x, subtext_y, 15, LIGHTGRAY);
}

void init(void *memory, unsigned long size) {
  util_init(memory, size);

  ds_string_slice fen = DS_STRING_SLICE(CHESS_START);
  chess_init_fen(&state, fen);

  ds_dynamic_array_init_allocator(&moves, sizeof(move_t), &allocator);
  chess_generate_moves(&state, &moves);

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chess Engine");

  InitAudioDevice();
}

void state_init(char *fen) {
  DS_MEMSET(&state, 0, sizeof(chess_state_t));

  ds_string_slice fen_slice = DS_STRING_SLICE(fen);
  chess_init_fen(&state, fen_slice);

  ds_dynamic_array_free(&moves);

  ds_dynamic_array_init_allocator(&moves, sizeof(move_t), &allocator);
  chess_generate_moves(&state, &moves);
}

void tick(float deltaTime) {
  UNUSED(deltaTime);

  BeginDrawing();

  ClearBackground(RAYWHITE);
  chess_print_board();
  if (checkmate_gui == 1) {
    chess_print_message("Checkmate!");
  } else if (stalemate_gui == 1) {
    chess_print_message("Stalemate!");
  } else if (draw_gui == 1) {
    chess_print_message("Draw!");
  } else {
    int index = -1;

    chess_move(&state, moves.items, moves.count, &index);

    if (index != -1) {
      move_t *move = NULL;
      ds_dynamic_array_get_ref(&moves, index, (void **)&move);

      chess_apply_move(&state, *move);

      if (move->move == CHESS_MOVE) {
        PlaySound(LoadSoundCachedMove(CHESS_MOVE));
      } else if ((move->move & CHESS_CAPTURE) != 0) {
        PlaySound(LoadSoundCachedMove(CHESS_CAPTURE));
      } else if ((move->move & CHESS_CASTLE_SHORT) != 0 ||
                 (move->move & CHESS_CASTLE_LONG) != 0) {
        PlaySound(LoadSoundCachedMove(CHESS_CASTLE_SHORT));
      }

      // In case of apply move we just re-draw the board to be safe
      ClearBackground(RAYWHITE);
      chess_print_board();

      state.current_player = chess_flip_player(state.current_player);

      is_in_check = chess_is_in_check(&state, state.current_player);

      chess_generate_moves(&state, &moves);

      if (checkmate_gui == 0 &&
          chess_is_checkmate(&state, state.current_player)) {
        checkmate_gui = 1;
      } else if (stalemate_gui == 0 &&
                 chess_is_stalemate(&state, state.current_player)) {
        stalemate_gui = 1;
      } else if (draw_gui == 0 && chess_is_draw(&state, state.current_player)) {
        draw_gui = 1;
      }

      chess_print_state();
    }
  }

  EndDrawing();
}

static int opponent_quit_gui = 0;

void tick_network(int dS, int my_color) {
  BeginDrawing();
  ClearBackground(RAYWHITE);
  
  // Vider le buffer réseau pour lire les infos (abandon de l'adversaire ou coup reçu)
  struct timeval timeout = {0, 0};

  while (1) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(dS, &set);
    
    if (select(dS + 1, &set, NULL, NULL, &timeout) <= 0) {
      break; // Aucun message ou erreur
    }
    
    MsgHeader header;
    char buf[512];
    int type = recv_message(dS, &header, buf, sizeof(buf));

    if (type <= 0) {
      opponent_quit_gui = 1;
      break;  // Prevent infinite loop on disconnect
    } else if (header.type == REP_RESULTAT_PARTIE) {
      str_RepResultatPartie *rep = (str_RepResultatPartie *)buf;
      if (rep->raison == GAME_RESIGN) {
        opponent_quit_gui = 1;
      }
    } else if (header.type == REP_MOVE) {
      str_RepMove *rep = (str_RepMove *)buf;

      if (state.current_player != my_color) {
        int index = -1;
        square_t s_start = MK_SQUARE(rep->dep_ligne, rep->dep_col);
        square_t s_end = MK_SQUARE(rep->vers_ligne, rep->vers_col);

        for (unsigned int i = 0; i < (unsigned int)moves.count; i++) {
          move_t *m = NULL;
          ds_dynamic_array_get_ref(&moves, i, (void **)&m);
          if (m->start.file == s_start.file &&
              m->start.rank == s_start.rank && m->end.file == s_end.file &&
              m->end.rank == s_end.rank) {
            index = i;
            break;
          }
        }

        if (index != -1) {
          move_t *move = NULL;
          ds_dynamic_array_get_ref(&moves, index, (void **)&move);

          chess_apply_move(&state, *move);
          if (move->move == CHESS_MOVE) {
            PlaySound(LoadSoundCachedMove(CHESS_MOVE));
          } else if ((move->move & CHESS_CAPTURE) != 0) {
            PlaySound(LoadSoundCachedMove(CHESS_CAPTURE));
          } else if ((move->move & CHESS_CASTLE_SHORT) != 0 ||
                     (move->move & CHESS_CASTLE_LONG) != 0) {
            PlaySound(LoadSoundCachedMove(CHESS_CASTLE_SHORT));
          }

          state.current_player = chess_flip_player(state.current_player);
          is_in_check = chess_is_in_check(&state, state.current_player);
          chess_generate_moves(&state, &moves);

          if (checkmate_gui == 0 &&
              chess_is_checkmate(&state, state.current_player)) {
            checkmate_gui = 1;
            str_ReqGameOver go;
            memset(&go, 0, sizeof(go));
            go.raison = GAME_CHECKMATE;
            // celui qui a mater l'adversaire gagne
            go.gagnant = (my_color == CHESS_WHITE) ? 1 : 0; 
            send_message(dS, REQ_GAME_OVER, &go, sizeof(go));
          } else if (stalemate_gui == 0 &&
                     chess_is_stalemate(&state, state.current_player)) {
            stalemate_gui = 1;
            str_ReqGameOver go;
            memset(&go, 0, sizeof(go));
            go.raison = GAME_STALEMATE;
            go.gagnant = 2; // nul
            send_message(dS, REQ_GAME_OVER, &go, sizeof(go));
          } else if (draw_gui == 0 &&
                     chess_is_draw(&state, state.current_player)) {
            draw_gui = 1;
            str_ReqGameOver go;
            memset(&go, 0, sizeof(go));
            go.raison = GAME_DRAW_50;
            go.gagnant = 2; // nul
            send_message(dS, REQ_GAME_OVER, &go, sizeof(go));
          }
        }
      }
    }
  }

  chess_print_board();
  if (checkmate_gui == 1) {
    chess_print_message("Checkmate!");
  } else if (stalemate_gui == 1) {
    chess_print_message("Stalemate!");
  } else if (draw_gui == 1) {
    chess_print_message("Draw!");
  } else if (opponent_quit_gui == 1) {
    chess_print_message("Opponent Quit!");
  } else {
    if (state.current_player == my_color) {
      int index = -1;
      chess_move(&state, moves.items, moves.count, &index);

      if (index != -1) {
        move_t *move = NULL;
        ds_dynamic_array_get_ref(&moves, index, (void **)&move);

        str_ReqMove req;
        memset(&req, 0, sizeof(req));
        req.dep_col = move->start.file;
        req.dep_ligne = move->start.rank;
        req.vers_col = move->end.file;
        req.vers_ligne = move->end.rank;

        if (send_message(dS, REQ_MOVE, &req, sizeof(req)) >= 0) {
          MsgHeader header;
          char buf[512];
          int type = recv_message(dS, &header, buf, sizeof(buf));
          if (type > 0 && header.type == REP_MOVE_ACK) {
            str_RepMove *rep = (str_RepMove *)buf;
            if (rep->resultat == STATUS_OK) {
              chess_apply_move(&state, *move);
              if (move->move == CHESS_MOVE) {
                PlaySound(LoadSoundCachedMove(CHESS_MOVE));
              } else if ((move->move & CHESS_CAPTURE) != 0) {
                PlaySound(LoadSoundCachedMove(CHESS_CAPTURE));
              } else if ((move->move & CHESS_CASTLE_SHORT) != 0 ||
                         (move->move & CHESS_CASTLE_LONG) != 0) {
                PlaySound(LoadSoundCachedMove(CHESS_CASTLE_SHORT));
              }

              state.current_player = chess_flip_player(state.current_player);
              is_in_check = chess_is_in_check(&state, state.current_player);
              chess_generate_moves(&state, &moves);

              if (checkmate_gui == 0 &&
                  chess_is_checkmate(&state, state.current_player)) {
                checkmate_gui = 1;
                // Signaler au serveur : je viens de mater l'adversaire
                str_ReqGameOver go;
                memset(&go, 0, sizeof(go));
                go.raison = GAME_CHECKMATE;
                go.gagnant = (my_color == CHESS_WHITE) ? 1 : 0;
                send_message(dS, REQ_GAME_OVER, &go, sizeof(go));
              } else if (stalemate_gui == 0 &&
                         chess_is_stalemate(&state, state.current_player)) {
                stalemate_gui = 1;
                str_ReqGameOver go;
                memset(&go, 0, sizeof(go));
                go.raison = GAME_STALEMATE;
                go.gagnant = 2; // nul
                send_message(dS, REQ_GAME_OVER, &go, sizeof(go));
              } else if (draw_gui == 0 &&
                         chess_is_draw(&state, state.current_player)) {
                draw_gui = 1;
                str_ReqGameOver go;
                memset(&go, 0, sizeof(go));
                go.raison = GAME_DRAW_50;
                go.gagnant = 2; // nul
                send_message(dS, REQ_GAME_OVER, &go, sizeof(go));
              }
            }
          } else if (type > 0 && header.type == REP_RESULTAT_PARTIE) {
            str_RepResultatPartie *rep = (str_RepResultatPartie *)buf;
            if (rep->raison == GAME_RESIGN) {
              opponent_quit_gui = 1;
            }
          } else if (type <= 0) {
            opponent_quit_gui = 1;
          }
        }
      }
    }
  }
  EndDrawing();
}

void game_network_play(int dS, int my_color) {
  chess_init(NULL, 0);
  init(NULL, 0);
  state_init(CHESS_START);

  // Réinitialiser les flags d'affichage pour la nouvelle partie
  checkmate_gui = 0;
  stalemate_gui = 0;
  draw_gui = 0;
  is_in_check = 0;
  opponent_quit_gui = 0;

  // Définition de la couleur pour le moteur
  int engine_color;
  if (my_color == 1) {
    engine_color = CHESS_WHITE;
  } else if (my_color == 0) {
    engine_color = CHESS_BLACK;
  } else {
    // Cas spectateur (my_color == -1)
    engine_color = -1;
  }

  while (!WindowShouldClose()) {
    tick_network(dS, engine_color);
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
      break;
    }
  }

  // Informe le serveur que le client (joueur ou spectateur) quitte la partie
  send_message(dS, REQ_RESIGN, NULL, 0);

  util_clean();
  CloseWindow();
}