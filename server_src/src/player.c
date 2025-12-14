/**
 * @file player.c
 * @brief Implementace spravy hracu
 */

#include "player.h"
#include "logger.h"
#include <string.h>
#include <unistd.h>

/* ============================================
 * IMPLEMENTACE
 * ============================================ */

void player_init_all(Player *players, int count) {
    for (int i = 0; i < count; i++) {
        memset(&players[i], 0, sizeof(Player));
        players[i].socket_fd = -1;
        players[i].room_id = -1;
        players[i].is_active = false;
        players[i].state = PLAYER_STATE_CONNECTING;
    }
}

void player_create(Player *player, int socket_fd) {
    memset(player, 0, sizeof(Player));
    player->socket_fd = socket_fd;
    player->state = PLAYER_STATE_CONNECTING;
    player->room_id = -1;
    player->skips_remaining = SKIPS_PER_PLAYER;
    player->recv_buffer_len = 0;
    player->last_activity = time(NULL);
    player->disconnect_time = 0;
    player->last_ping = 0;
    player->waiting_pong = false;
    player->invalid_message_count = 0;
    player->is_active = true;
    player->nickname[0] = '\0';
}

void player_reset(Player *player, bool keep_for_reconnect) {
    if (player->socket_fd >= 0) {
        close(player->socket_fd);
    }
    
    if (keep_for_reconnect) {
        /* Zachovame identitu a hernni stav pro reconnect */
        player->socket_fd = -1;
        player->state = PLAYER_STATE_DISCONNECTED;
        player->disconnect_time = time(NULL);
        player->recv_buffer_len = 0;
        player->waiting_pong = false;
        player->invalid_message_count = 0;
    } else {
        /* Uplny reset */
        int room_id = player->room_id; /* Pro pozdejsi uklid */
        memset(player, 0, sizeof(Player));
        player->socket_fd = -1;
        player->room_id = -1;
        player->is_active = false;
        player->state = PLAYER_STATE_CONNECTING;
        (void)room_id; /* Pouzit v lobby/room modulu */
    }
}

int player_find_free_slot(Player *players, int count) {
    for (int i = 0; i < count; i++) {
        if (!players[i].is_active) {
            return i;
        }
    }
    return -1;
}

Player* player_find_by_socket(Player *players, int count, int socket_fd) {
    for (int i = 0; i < count; i++) {
        if (players[i].is_active && players[i].socket_fd == socket_fd) {
            return &players[i];
        }
    }
    return NULL;
}

Player* player_find_by_nickname(Player *players, int count, const char *nickname) {
    if (nickname == NULL) return NULL;
    
    for (int i = 0; i < count; i++) {
        if (players[i].is_active && 
            strlen(players[i].nickname) > 0 &&
            strcmp(players[i].nickname, nickname) == 0) {
            return &players[i];
        }
    }
    return NULL;
}

Player* player_find_disconnected(Player *players, int count, const char *nickname) {
    if (nickname == NULL) return NULL;
    
    for (int i = 0; i < count; i++) {
        if (players[i].is_active && 
            players[i].state == PLAYER_STATE_DISCONNECTED &&
            strcmp(players[i].nickname, nickname) == 0) {
            return &players[i];
        }
    }
    return NULL;
}

void player_set_nickname(Player *player, const char *nickname) {
    if (player && nickname) {
        strncpy(player->nickname, nickname, MAX_NICKNAME_LENGTH);
        player->nickname[MAX_NICKNAME_LENGTH] = '\0';
    }
}

void player_set_state(Player *player, PlayerState state) {
    if (player) {
        LOG_DEBUG("Player '%s' state: %s -> %s",
                  player->nickname[0] ? player->nickname : "(unknown)",
                  player_state_to_string(player->state),
                  player_state_to_string(state));
        player->state = state;
    }
}

void player_update_activity(Player *player) {
    if (player) {
        player->last_activity = time(NULL);
    }
}

bool player_reconnect_timeout_expired(Player *player) {
    if (player == NULL || player->state != PLAYER_STATE_DISCONNECTED) {
        return false;
    }
    
    time_t now = time(NULL);
    return (now - player->disconnect_time) > SHORT_DISCONNECT_TIMEOUT;
}

bool player_needs_ping(Player *player) {
    if (player == NULL || player->socket_fd < 0) {
        return false;
    }
    
    time_t now = time(NULL);
    return !player->waiting_pong && 
           (now - player->last_activity) > PING_INTERVAL;
}

bool player_pong_timeout_expired(Player *player) {
    if (player == NULL || !player->waiting_pong) {
        return false;
    }
    
    time_t now = time(NULL);
    return (now - player->last_ping) > PING_TIMEOUT;
}

int player_count_active(Player *players, int count) {
    int active = 0;
    for (int i = 0; i < count; i++) {
        if (players[i].is_active && players[i].state != PLAYER_STATE_DISCONNECTED) {
            active++;
        }
    }
    return active;
}

const char* player_state_to_string(PlayerState state) {
    switch (state) {
        case PLAYER_STATE_CONNECTING:   return "CONNECTING";
        case PLAYER_STATE_LOBBY:        return "LOBBY";
        case PLAYER_STATE_IN_ROOM:      return "IN_ROOM";
        case PLAYER_STATE_IN_GAME:      return "IN_GAME";
        case PLAYER_STATE_DISCONNECTED: return "DISCONNECTED";
        default:                        return "UNKNOWN";
    }
}

