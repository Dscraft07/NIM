/**
 * @file room.c
 * @brief Implementace spravy hernich mistnosti
 */

#include "room.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>

/* ============================================
 * PRIVATNI FUNKCE
 * ============================================ */

/**
 * Najde volny slot pro mistnost
 */
static int find_free_slot(Room *rooms, int count) {
    for (int i = 0; i < count; i++) {
        if (!rooms[i].is_active) {
            return i;
        }
    }
    return -1;
}

/* ============================================
 * IMPLEMENTACE VEREJNYCH FUNKCI
 * ============================================ */

void room_init_all(Room *rooms, int count) {
    for (int i = 0; i < count; i++) {
        memset(&rooms[i], 0, sizeof(Room));
        rooms[i].id = -1;
        rooms[i].is_active = false;
        rooms[i].player_count = 0;
        for (int j = 0; j < PLAYERS_PER_ROOM; j++) {
            rooms[i].players[j] = NULL;
        }
        game_init(&rooms[i].game);
    }
}

int room_create(Room *rooms, int count, const char *name, Player *creator) {
    if (rooms == NULL || name == NULL || creator == NULL) {
        return -1;
    }
    
    /* Kontrola, zda nazev neni obsazen */
    if (room_find_by_name(rooms, count, name) != NULL) {
        LOG_WARNING("Room name '%s' already taken", name);
        return -1;
    }
    
    /* Najdi volny slot */
    int slot = find_free_slot(rooms, count);
    if (slot < 0) {
        LOG_WARNING("No free room slots available");
        return -1;
    }
    
    /* Inicializace mistnosti */
    Room *room = &rooms[slot];
    room->id = slot;
    strncpy(room->name, name, MAX_ROOM_NAME_LENGTH);
    room->name[MAX_ROOM_NAME_LENGTH] = '\0';
    room->is_active = true;
    room->player_count = 0;
    
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        room->players[i] = NULL;
    }
    
    game_init(&room->game);
    
    /* Pridani tvurce */
    if (!room_add_player(room, creator)) {
        room->is_active = false;
        return -1;
    }
    
    LOG_INFO("Room '%s' (ID: %d) created by '%s'", 
             room->name, room->id, creator->nickname);
    
    return room->id;
}

Room* room_find_by_id(Room *rooms, int count, int id) {
    if (rooms == NULL || id < 0 || id >= count) {
        return NULL;
    }
    
    if (rooms[id].is_active) {
        return &rooms[id];
    }
    
    return NULL;
}

Room* room_find_by_name(Room *rooms, int count, const char *name) {
    if (rooms == NULL || name == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < count; i++) {
        if (rooms[i].is_active && strcmp(rooms[i].name, name) == 0) {
            return &rooms[i];
        }
    }
    
    return NULL;
}

bool room_add_player(Room *room, Player *player) {
    if (room == NULL || player == NULL) {
        return false;
    }
    
    if (room_is_full(room)) {
        LOG_WARNING("Cannot add player to full room %d", room->id);
        return false;
    }
    
    /* Najdi volny slot */
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->players[i] == NULL) {
            room->players[i] = player;
            room->player_count++;
            player->room_id = room->id;
            player->skips_remaining = SKIPS_PER_PLAYER;
            
            LOG_INFO("Player '%s' joined room '%s' (ID: %d)", 
                     player->nickname, room->name, room->id);
            
            return true;
        }
    }
    
    return false;
}

bool room_remove_player(Room *room, Player *player) {
    if (room == NULL || player == NULL) {
        return false;
    }
    
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->players[i] == player) {
            room->players[i] = NULL;
            room->player_count--;
            player->room_id = -1;
            
            LOG_INFO("Player '%s' left room '%s' (ID: %d)", 
                     player->nickname, room->name, room->id);
            
            /* Pokud je mistnost prazdna, zrus ji */
            if (room_is_empty(room)) {
                room_destroy(room);
            }
            
            return true;
        }
    }
    
    return false;
}

int room_get_player_index(Room *room, Player *player) {
    if (room == NULL || player == NULL) {
        return -1;
    }
    
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->players[i] == player) {
            return i;
        }
    }
    
    return -1;
}

Player* room_get_opponent(Room *room, Player *player) {
    if (room == NULL || player == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->players[i] != NULL && room->players[i] != player) {
            return room->players[i];
        }
    }
    
    return NULL;
}

bool room_is_full(Room *room) {
    return room != NULL && room->player_count >= PLAYERS_PER_ROOM;
}

bool room_is_empty(Room *room) {
    return room != NULL && room->player_count == 0;
}

void room_destroy(Room *room) {
    if (room == NULL) return;
    
    LOG_INFO("Room '%s' (ID: %d) destroyed", room->name, room->id);
    
    /* Vrat hrace do lobby */
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->players[i] != NULL) {
            room->players[i]->room_id = -1;
            room->players[i] = NULL;
        }
    }
    
    room->is_active = false;
    room->player_count = 0;
    room->id = -1;
    room->name[0] = '\0';
    game_reset(&room->game);
}

int room_count_active(Room *rooms, int count) {
    int active = 0;
    for (int i = 0; i < count; i++) {
        if (rooms[i].is_active) {
            active++;
        }
    }
    return active;
}

int room_list_to_string(Room *rooms, int count, char *buffer, int size) {
    if (rooms == NULL || buffer == NULL || size <= 0) {
        return 0;
    }
    
    int active_count = room_count_active(rooms, count);
    int written = snprintf(buffer, size, "%d", active_count);
    
    if (active_count == 0) {
        return written;
    }
    
    for (int i = 0; i < count && written < size - 1; i++) {
        if (rooms[i].is_active) {
            written += snprintf(buffer + written, size - written,
                               ";%d,%s,%d,%d",
                               rooms[i].id,
                               rooms[i].name,
                               rooms[i].player_count,
                               PLAYERS_PER_ROOM);
        }
    }
    
    return written;
}

bool room_start_game(Room *room) {
    if (room == NULL) return false;
    
    if (!room_is_full(room)) {
        LOG_WARNING("Cannot start game - room not full");
        return false;
    }
    
    if (room->game.state != GAME_STATE_WAITING) {
        LOG_WARNING("Cannot start game - game already in progress or finished");
        return false;
    }
    
    game_start(&room->game);
    
    /* Nastav hrace do stavu IN_GAME */
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        if (room->players[i] != NULL) {
            player_set_state(room->players[i], PLAYER_STATE_IN_GAME);
            room->players[i]->skips_remaining = SKIPS_PER_PLAYER;
        }
    }
    
    LOG_INFO("Game started in room '%s' (ID: %d)", room->name, room->id);
    
    return true;
}

