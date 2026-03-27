#include "stdlib.h"
#include "raylib.h"
#include "chess.h"
#include "game.h"
#include "human.h"
#include "ds.h"

#define SUBCMD_GAME "game"
#define SUBCMD_COUNT_POSITIONS "count-positions"

typedef struct arguments_t {
    char *subcmd;
    int depth;
    char *fen;
} arguments_t;

void parse_arguments(int argc, char **argv, arguments_t *args) {
    ds_argparse_parser parser = {0};
    ds_argparse_parser_init(
        &parser,
        "chess-engine",
        "Chess Engine in C",
        "0.1"
    );

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 's',
        .long_name = "subcmd",
        .description = "The subcommand to use with the engine: `game`, `count-positions`. Default: `game`",
        .type = ARGUMENT_TYPE_POSITIONAL,
        .required = false,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'd',
        .long_name = "depth",
        .description = "The depth to use with the engine. Default: `0`.",
        .type = ARGUMENT_TYPE_VALUE,
        .required = false,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'f',
        .long_name = "fen",
        .description = "the FEN string to use for the game",
        .type = ARGUMENT_TYPE_VALUE,
        .required = false,
    });

    DS_UNREACHABLE(ds_argparse_parse(&parser, argc, argv));

    args->subcmd = ds_argparse_get_value_or_default(&parser, "subcmd", SUBCMD_GAME);
    args->depth = atoi(ds_argparse_get_value_or_default(&parser, "depth", "0"));
    args->fen = ds_argparse_get_value_or_default(&parser, "fen", CHESS_START);

    ds_argparse_parser_free(&parser);
}

int game(arguments_t args) {
    chess_init(NULL, 0);

    init(NULL, 0);
    state_init(args.fen);

    while (!WindowShouldClose()) {
        tick(GetFrameTime());
    }

    CloseWindow();

    return 0;
}

int count_positions(arguments_t args) {
    chess_state_t state = {0};

    ds_string_slice fen = DS_STRING_SLICE(args.fen);
    chess_init_fen(&state, fen);

    perft_t perft = {0};
    chess_count_positions(&state, args.depth, &perft);
    DS_LOG_INFO("Depth: %d => Positions: %d Captures: %d E.p.: %d Castles: %d "
                "Promotions: %d Checks: %d Checkmates: %d",
                args.depth, perft.nodes, perft.captures, perft.enp,
                perft.castles, perft.promote, perft.checks, perft.checkmates);

    return 0;
}

int main(int argc, char **argv) {
    arguments_t args = {0};
    parse_arguments(argc, argv, &args);

    if (DS_STRCMP(args.subcmd, SUBCMD_GAME) == 0) {
        return game(args);
    } else if (DS_STRCMP(args.subcmd, SUBCMD_COUNT_POSITIONS) == 0) {
        return count_positions(args);
    }

    return 1;
}
