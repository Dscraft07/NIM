/**
 * @file game.h
 * @brief Herni logika Nim
 */

#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "../include/config.h"

/* ============================================
 * STAVY HRY
 * ============================================ */

typedef enum {
    GAME_STATE_WAITING,     /* Ceka se na druheho hrace */
    GAME_STATE_PLAYING,     /* Hra probiha */
    GAME_STATE_PAUSED,      /* Hra pozastavena (hrac odpojen) */
    GAME_STATE_FINISHED     /* Hra skoncila */
} GameState;

/* ============================================
 * STRUKTURA HRY
 * ============================================ */

typedef struct {
    GameState state;                /* Stav hry */
    int stones;                     /* Pocet zbyvajicich kaminku */
    int current_player;             /* Index aktualniho hrace (0 nebo 1) */
    int player_skips[2];            /* Zbyvajici preskoceni pro kazdeho hrace */
    int winner;                     /* Index viteze (-1 = jeste neni) */
} Game;

/* ============================================
 * VEREJNE FUNKCE
 * ============================================ */

/**
 * Inicializuje novou hru
 * @param game Ukazatel na strukturu hry
 */
void game_init(Game *game);

/**
 * Zacne hru (kdyz jsou 2 hraci)
 * @param game Ukazatel na hru
 */
void game_start(Game *game);

/**
 * Resetuje hru do vychoziho stavu
 * @param game Ukazatel na hru
 */
void game_reset(Game *game);

/**
 * Provede tah - odebrani kaminku
 * @param game Ukazatel na hru
 * @param player_index Index hrace (0 nebo 1)
 * @param count Pocet kaminku k odebrani
 * @return true pri uspechu, false pri neplatnem tahu
 */
bool game_take_stones(Game *game, int player_index, int count);

/**
 * Provede preskoceni tahu
 * @param game Ukazatel na hru
 * @param player_index Index hrace (0 nebo 1)
 * @return true pri uspechu, false pokud nelze
 */
bool game_skip_turn(Game *game, int player_index);

/**
 * Zkontroluje, zda hra skoncila
 * @param game Ukazatel na hru
 * @return true pokud hra skoncila
 */
bool game_is_over(Game *game);

/**
 * Zkontroluje, zda je tah hrace na rade
 * @param game Ukazatel na hru
 * @param player_index Index hrace
 * @return true pokud je hrac na tahu
 */
bool game_is_player_turn(Game *game, int player_index);

/**
 * Zkontroluje, zda muze hrac preskocit
 * @param game Ukazatel na hru
 * @param player_index Index hrace
 * @return true pokud muze preskocit
 */
bool game_can_skip(Game *game, int player_index);

/**
 * Validuje pocet kaminku k odebrani
 * @param game Ukazatel na hru
 * @param count Pocet kaminku
 * @return true pokud je pocet validni
 */
bool game_validate_take_count(Game *game, int count);

/**
 * Pozastavi hru (pri odpojeni hrace)
 * @param game Ukazatel na hru
 */
void game_pause(Game *game);

/**
 * Obnovi hru (pri reconnectu)
 * @param game Ukazatel na hru
 */
void game_resume(Game *game);

/**
 * Vrati pocet zbyvajicich kaminku
 * @param game Ukazatel na hru
 * @return Pocet kaminku
 */
int game_get_stones(Game *game);

/**
 * Vrati index aktualniho hrace
 * @param game Ukazatel na hru
 * @return Index hrace (0 nebo 1)
 */
int game_get_current_player(Game *game);

/**
 * Vrati index viteze
 * @param game Ukazatel na hru
 * @return Index viteze nebo -1
 */
int game_get_winner(Game *game);

/**
 * Vrati index porazeneho
 * @param game Ukazatel na hru
 * @return Index porazeneho nebo -1
 */
int game_get_loser(Game *game);

/**
 * Vrati textovou reprezentaci stavu hry
 * @param state Stav hry
 * @return Textova reprezentace
 */
const char* game_state_to_string(GameState state);

#endif /* GAME_H */

