/**
 * @file game.c
 * @brief Implementace herni logiky Nim
 * 
 * Pravidla hry:
 * - Zacina se s INITIAL_STONES kaminky
 * - Hraci se stridaji, kazdy odebira MIN_TAKE az MAX_TAKE kaminku
 * - Kazdy hrac muze jednou preskocit tah
 * - Kdo odebere posledni kaminek, prohrává (misere)
 */

#include "game.h"
#include "logger.h"
#include <string.h>

/* ============================================
 * IMPLEMENTACE
 * ============================================ */

void game_init(Game *game) {
    if (game == NULL) return;
    
    memset(game, 0, sizeof(Game));
    game->state = GAME_STATE_WAITING;
    game->stones = INITIAL_STONES;
    game->current_player = 0;
    game->player_skips[0] = SKIPS_PER_PLAYER;
    game->player_skips[1] = SKIPS_PER_PLAYER;
    game->winner = -1;
}

void game_start(Game *game) {
    if (game == NULL) return;
    
    game->state = GAME_STATE_PLAYING;
    game->stones = INITIAL_STONES;
    game->current_player = 0; /* Prvni hrac zacina */
    game->player_skips[0] = SKIPS_PER_PLAYER;
    game->player_skips[1] = SKIPS_PER_PLAYER;
    game->winner = -1;
    
    LOG_INFO("Game started with %d stones", game->stones);
}

void game_reset(Game *game) {
    game_init(game);
}

bool game_take_stones(Game *game, int player_index, int count) {
    if (game == NULL) return false;
    
    /* Kontrola stavu hry */
    if (game->state != GAME_STATE_PLAYING) {
        LOG_WARNING("Cannot take stones - game not in PLAYING state");
        return false;
    }
    
    /* Kontrola, zda je hrac na tahu */
    if (game->current_player != player_index) {
        LOG_WARNING("Player %d tried to take stones, but it's player %d's turn",
                    player_index, game->current_player);
        return false;
    }
    
    /* Validace poctu */
    if (!game_validate_take_count(game, count)) {
        LOG_WARNING("Invalid take count: %d (stones: %d, min: %d, max: %d)",
                    count, game->stones, MIN_TAKE, MAX_TAKE);
        return false;
    }
    
    /* Odebrani kaminku */
    game->stones -= count;
    LOG_DEBUG("Player %d took %d stones, %d remaining", 
              player_index, count, game->stones);
    
    /* Kontrola konce hry - kdo vzal posledni, prohrává */
    if (game->stones == 0) {
        game->state = GAME_STATE_FINISHED;
        game->winner = 1 - player_index; /* Druhy hrac vyhrává */
        LOG_INFO("Game over! Player %d wins (player %d took last stone)",
                 game->winner, player_index);
        return true;
    }
    
    /* Predani tahu */
    game->current_player = 1 - game->current_player;
    
    return true;
}

bool game_skip_turn(Game *game, int player_index) {
    if (game == NULL) return false;
    
    /* Kontrola stavu hry */
    if (game->state != GAME_STATE_PLAYING) {
        LOG_WARNING("Cannot skip - game not in PLAYING state");
        return false;
    }
    
    /* Kontrola, zda je hrac na tahu */
    if (game->current_player != player_index) {
        LOG_WARNING("Player %d tried to skip, but it's player %d's turn",
                    player_index, game->current_player);
        return false;
    }
    
    /* Kontrola, zda ma hrac preskoceni */
    if (game->player_skips[player_index] <= 0) {
        LOG_WARNING("Player %d has no skips remaining", player_index);
        return false;
    }
    
    /* Preskoceni tahu */
    game->player_skips[player_index]--;
    game->current_player = 1 - game->current_player;
    
    LOG_DEBUG("Player %d skipped turn, %d skips remaining",
              player_index, game->player_skips[player_index]);
    
    return true;
}

bool game_is_over(Game *game) {
    return game != NULL && game->state == GAME_STATE_FINISHED;
}

bool game_is_player_turn(Game *game, int player_index) {
    if (game == NULL) return false;
    return game->state == GAME_STATE_PLAYING && 
           game->current_player == player_index;
}

bool game_can_skip(Game *game, int player_index) {
    if (game == NULL) return false;
    return game->player_skips[player_index] > 0;
}

bool game_validate_take_count(Game *game, int count) {
    if (game == NULL) return false;
    
    /* Kontrola rozsahu */
    if (count < MIN_TAKE || count > MAX_TAKE) {
        return false;
    }
    
    /* Kontrola, zda je dost kaminku */
    if (count > game->stones) {
        return false;
    }
    
    return true;
}

void game_pause(Game *game) {
    if (game != NULL && game->state == GAME_STATE_PLAYING) {
        game->state = GAME_STATE_PAUSED;
        LOG_INFO("Game paused");
    }
}

void game_resume(Game *game) {
    if (game != NULL && game->state == GAME_STATE_PAUSED) {
        game->state = GAME_STATE_PLAYING;
        LOG_INFO("Game resumed");
    }
}

int game_get_stones(Game *game) {
    return game ? game->stones : 0;
}

int game_get_current_player(Game *game) {
    return game ? game->current_player : -1;
}

int game_get_winner(Game *game) {
    return game ? game->winner : -1;
}

int game_get_loser(Game *game) {
    if (game == NULL || game->winner < 0) return -1;
    return 1 - game->winner;
}

const char* game_state_to_string(GameState state) {
    switch (state) {
        case GAME_STATE_WAITING:  return "WAITING";
        case GAME_STATE_PLAYING:  return "PLAYING";
        case GAME_STATE_PAUSED:   return "PAUSED";
        case GAME_STATE_FINISHED: return "FINISHED";
        default:                  return "UNKNOWN";
    }
}

