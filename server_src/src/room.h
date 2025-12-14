/**
 * @file room.h
 * @brief Sprava hernich mistnosti
 */

#ifndef ROOM_H
#define ROOM_H

#include <stdbool.h>
#include "game.h"
#include "player.h"
#include "../include/config.h"

/* ============================================
 * STRUKTURA MISTNOSTI
 * ============================================ */

typedef struct {
    int id;                                     /* ID mistnosti */
    char name[MAX_ROOM_NAME_LENGTH + 1];        /* Nazev mistnosti */
    Player *players[PLAYERS_PER_ROOM];          /* Ukazatele na hrace */
    int player_count;                           /* Pocet hracu */
    Game game;                                  /* Stav hry */
    bool is_active;                             /* Je mistnost aktivni? */
} Room;

/* ============================================
 * VEREJNE FUNKCE
 * ============================================ */

/**
 * Inicializuje pole mistnosti
 * @param rooms Pole mistnosti
 * @param count Pocet mistnosti
 */
void room_init_all(Room *rooms, int count);

/**
 * Vytvori novou mistnost
 * @param rooms Pole mistnosti
 * @param count Pocet mistnosti
 * @param name Nazev mistnosti
 * @param creator Hrac, ktery vytvari mistnost
 * @return ID nove mistnosti nebo -1 pri chybe
 */
int room_create(Room *rooms, int count, const char *name, Player *creator);

/**
 * Najde mistnost podle ID
 * @param rooms Pole mistnosti
 * @param count Pocet mistnosti
 * @param id ID mistnosti
 * @return Ukazatel na mistnost nebo NULL
 */
Room* room_find_by_id(Room *rooms, int count, int id);

/**
 * Najde mistnost podle nazvu
 * @param rooms Pole mistnosti
 * @param count Pocet mistnosti
 * @param name Nazev mistnosti
 * @return Ukazatel na mistnost nebo NULL
 */
Room* room_find_by_name(Room *rooms, int count, const char *name);

/**
 * Prida hrace do mistnosti
 * @param room Ukazatel na mistnost
 * @param player Hrac
 * @return true pri uspechu
 */
bool room_add_player(Room *room, Player *player);

/**
 * Odebere hrace z mistnosti
 * @param room Ukazatel na mistnost
 * @param player Hrac
 * @return true pri uspechu
 */
bool room_remove_player(Room *room, Player *player);

/**
 * Vrati index hrace v mistnosti
 * @param room Ukazatel na mistnost
 * @param player Hrac
 * @return Index (0 nebo 1) nebo -1 pokud neni v mistnosti
 */
int room_get_player_index(Room *room, Player *player);

/**
 * Vrati protihrace
 * @param room Ukazatel na mistnost
 * @param player Hrac
 * @return Ukazatel na protihrace nebo NULL
 */
Player* room_get_opponent(Room *room, Player *player);

/**
 * Zkontroluje, zda je mistnost plna
 * @param room Ukazatel na mistnost
 * @return true pokud je plna
 */
bool room_is_full(Room *room);

/**
 * Zkontroluje, zda je mistnost prazdna
 * @param room Ukazatel na mistnost
 * @return true pokud je prazdna
 */
bool room_is_empty(Room *room);

/**
 * Zrusi mistnost
 * @param room Ukazatel na mistnost
 */
void room_destroy(Room *room);

/**
 * Spocita aktivni mistnosti
 * @param rooms Pole mistnosti
 * @param count Pocet slotu
 * @return Pocet aktivnich mistnosti
 */
int room_count_active(Room *rooms, int count);

/**
 * Vytvori retezec se seznamem mistnosti pro protokol
 * Format: count;id1,name1,players1,max1;id2,...
 * @param rooms Pole mistnosti
 * @param count Pocet slotu
 * @param buffer Vystupni buffer
 * @param size Velikost bufferu
 * @return Delka retezce
 */
int room_list_to_string(Room *rooms, int count, char *buffer, int size);

/**
 * Zacne hru v mistnosti (pokud jsou 2 hraci)
 * @param room Ukazatel na mistnost
 * @return true pokud hra zacala
 */
bool room_start_game(Room *room);

#endif /* ROOM_H */

