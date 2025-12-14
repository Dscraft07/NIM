/**
 * @file player.h
 * @brief Sprava hracu a jejich stavu
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include <time.h>
#include "../include/config.h"

/* ============================================
 * STAVY HRACE
 * ============================================ */

typedef enum {
    PLAYER_STATE_CONNECTING,    /* Prave se pripojil, ceka na LOGIN */
    PLAYER_STATE_LOBBY,         /* V lobby, muze vstoupit do mistnosti */
    PLAYER_STATE_IN_ROOM,       /* V mistnosti, ceka na protihrace */
    PLAYER_STATE_IN_GAME,       /* Ve hre */
    PLAYER_STATE_DISCONNECTED   /* Docasne odpojen (muze se vratit) */
} PlayerState;

/* ============================================
 * STRUKTURA HRACE
 * ============================================ */

typedef struct {
    int socket_fd;                          /* Socket descriptor (-1 = odpojen) */
    char nickname[MAX_NICKNAME_LENGTH + 1]; /* Prezdivka */
    PlayerState state;                      /* Aktualni stav */
    int room_id;                            /* ID mistnosti (-1 = neni v mistnosti) */
    
    /* Herni data */
    int skips_remaining;                    /* Pocet zbyvajicich preskoceni */
    
    /* Sitova data */
    char recv_buffer[BUFFER_SIZE];          /* Buffer pro prijimani dat */
    int recv_buffer_len;                    /* Delka dat v bufferu */
    
    /* Casove udaje */
    time_t last_activity;                   /* Cas posledni aktivity */
    time_t disconnect_time;                 /* Cas odpojeni (pro reconnect) */
    time_t last_ping;                       /* Cas posledniho PING */
    bool waiting_pong;                      /* Cekame na PONG? */
    
    /* Validace */
    int invalid_message_count;              /* Pocet nevalidnich zprav */
    
    /* Rate limiting */
    int messages_this_second;               /* Pocet zprav v aktualni sekunde */
    time_t rate_limit_second;               /* Sekunda pro rate limiting */
    
    /* Priznaky */
    bool is_active;                         /* Je slot aktivni? */
} Player;

/* ============================================
 * VEREJNE FUNKCE
 * ============================================ */

/**
 * Inicializuje pole hracu
 * @param players Pole hracu
 * @param count Pocet slotu
 */
void player_init_all(Player *players, int count);

/**
 * Vytvori noveho hrace
 * @param player Ukazatel na slot hrace
 * @param socket_fd Socket descriptor
 */
void player_create(Player *player, int socket_fd);

/**
 * Resetuje hrace do vychoziho stavu (pri odpojeni)
 * @param player Ukazatel na hrace
 * @param keep_for_reconnect Zachovat pro mozny reconnect?
 */
void player_reset(Player *player, bool keep_for_reconnect);

/**
 * Najde volny slot pro hrace
 * @param players Pole hracu
 * @param count Pocet slotu
 * @return Index volneho slotu nebo -1
 */
int player_find_free_slot(Player *players, int count);

/**
 * Najde hrace podle socketu
 * @param players Pole hracu
 * @param count Pocet slotu
 * @param socket_fd Socket descriptor
 * @return Ukazatel na hrace nebo NULL
 */
Player* player_find_by_socket(Player *players, int count, int socket_fd);

/**
 * Najde hrace podle prezdivky
 * @param players Pole hracu
 * @param count Pocet slotu
 * @param nickname Prezdivka
 * @return Ukazatel na hrace nebo NULL
 */
Player* player_find_by_nickname(Player *players, int count, const char *nickname);

/**
 * Najde odpojeneho hrace podle prezdivky (pro reconnect)
 * @param players Pole hracu
 * @param count Pocet slotu
 * @param nickname Prezdivka
 * @return Ukazatel na hrace nebo NULL
 */
Player* player_find_disconnected(Player *players, int count, const char *nickname);

/**
 * Nastavi prezdivku hraci
 * @param player Ukazatel na hrace
 * @param nickname Nova prezdivka
 */
void player_set_nickname(Player *player, const char *nickname);

/**
 * Nastavi stav hrace
 * @param player Ukazatel na hrace
 * @param state Novy stav
 */
void player_set_state(Player *player, PlayerState state);

/**
 * Aktualizuje cas posledni aktivity
 * @param player Ukazatel na hrace
 */
void player_update_activity(Player *player);

/**
 * Zkontroluje, zda hrac prekrocil timeout pro reconnect
 * @param player Ukazatel na hrace
 * @return true pokud prekrocil timeout
 */
bool player_reconnect_timeout_expired(Player *player);

/**
 * Zkontroluje, zda hrac potrebuje PING
 * @param player Ukazatel na hrace
 * @return true pokud potrebuje PING
 */
bool player_needs_ping(Player *player);

/**
 * Zkontroluje, zda hrac prekrocil PONG timeout
 * @param player Ukazatel na hrace
 * @return true pokud prekrocil timeout
 */
bool player_pong_timeout_expired(Player *player);

/**
 * Pocet aktivnich hracu
 * @param players Pole hracu
 * @param count Pocet slotu
 * @return Pocet aktivnich hracu
 */
int player_count_active(Player *players, int count);

/**
 * Vrati textovou reprezentaci stavu hrace
 * @param state Stav
 * @return Textova reprezentace
 */
const char* player_state_to_string(PlayerState state);

#endif /* PLAYER_H */

