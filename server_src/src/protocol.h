/**
 * @file protocol.h
 * @brief Definice protokolu pro komunikaci klient-server
 * 
 * Textovy protokol nad TCP.
 * Format zpravy: COMMAND;param1;param2;...\n
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>

/* ============================================
 * TYPY ZPRAV (COMMANDS)
 * ============================================ */

typedef enum {
    /* Klientske zpravy */
    MSG_LOGIN,          /* LOGIN;nickname */
    MSG_LIST_ROOMS,     /* LIST_ROOMS */
    MSG_CREATE_ROOM,    /* CREATE_ROOM;name */
    MSG_JOIN_ROOM,      /* JOIN_ROOM;room_id */
    MSG_LEAVE_ROOM,     /* LEAVE_ROOM */
    MSG_TAKE,           /* TAKE;count */
    MSG_SKIP,           /* SKIP */
    MSG_PING,           /* PING */
    MSG_LOGOUT,         /* LOGOUT */
    
    /* Serverove zpravy */
    MSG_LOGIN_OK,       /* LOGIN_OK */
    MSG_LOGIN_ERR,      /* LOGIN_ERR;reason */
    MSG_ROOMS,          /* ROOMS;count;id,name,players,max;... */
    MSG_ROOM_CREATED,   /* ROOM_CREATED;room_id */
    MSG_ROOM_JOINED,    /* ROOM_JOINED;room_id;opponent_or_empty */
    MSG_ROOM_ERR,       /* ROOM_ERR;reason */
    MSG_LEAVE_OK,       /* LEAVE_OK */
    MSG_GAME_START,     /* GAME_START;stones;your_turn;opponent_nick */
    MSG_TAKE_OK,        /* TAKE_OK;remaining;next_player */
    MSG_TAKE_ERR,       /* TAKE_ERR;reason */
    MSG_SKIP_OK,        /* SKIP_OK;next_player */
    MSG_SKIP_ERR,       /* SKIP_ERR;reason */
    MSG_OPPONENT_ACTION,/* OPPONENT_ACTION;action;param;remaining */
    MSG_GAME_OVER,      /* GAME_OVER;winner;loser */
    MSG_PONG,           /* PONG */
    MSG_PLAYER_STATUS,  /* PLAYER_STATUS;nickname;status */
    MSG_ERROR,          /* ERROR;code;message */
    MSG_SERVER_SHUTDOWN,/* SERVER_SHUTDOWN */
    MSG_WAIT_OPPONENT,  /* WAIT_OPPONENT */
    MSG_GAME_RESUMED,   /* GAME_RESUMED;stones;your_turn;your_skips;opp_skips */
    
    /* Specialni */
    MSG_UNKNOWN         /* Neznama zprava */
} MessageType;

/* ============================================
 * CHYBOVE KODY
 * ============================================ */

typedef enum {
    ERR_NONE = 0,
    ERR_INVALID_FORMAT = 1,      /* Nespravny format zpravy */
    ERR_UNKNOWN_COMMAND = 2,     /* Neznamy prikaz */
    ERR_INVALID_PARAMS = 3,      /* Nespravne parametry */
    ERR_NOT_LOGGED_IN = 4,       /* Hrac neni prihlasen */
    ERR_ALREADY_LOGGED_IN = 5,   /* Hrac uz je prihlasen */
    ERR_NICKNAME_TAKEN = 6,      /* Prezdivka je obsazena */
    ERR_NICKNAME_INVALID = 7,    /* Neplatna prezdivka */
    ERR_ROOM_NOT_FOUND = 8,      /* Mistnost neexistuje */
    ERR_ROOM_FULL = 9,           /* Mistnost je plna */
    ERR_ROOM_NAME_TAKEN = 10,    /* Nazev mistnosti je obsazen */
    ERR_NOT_IN_ROOM = 11,        /* Hrac neni v mistnosti */
    ERR_NOT_IN_GAME = 12,        /* Hrac neni ve hre */
    ERR_NOT_YOUR_TURN = 13,      /* Neni tvuj tah */
    ERR_INVALID_MOVE = 14,       /* Neplatny tah */
    ERR_NO_SKIPS_LEFT = 15,      /* Zadne preskoceni nezbyva */
    ERR_SERVER_FULL = 16,        /* Server je plny */
    ERR_MAX_ROOMS = 17,          /* Maximalni pocet mistnosti */
    ERR_GAME_IN_PROGRESS = 18,   /* Hra uz probiha */
    ERR_INTERNAL = 99            /* Interni chyba serveru */
} ErrorCode;

/* ============================================
 * STATUS HRACE
 * ============================================ */

typedef enum {
    STATUS_CONNECTED,
    STATUS_DISCONNECTED,
    STATUS_RECONNECTED
} PlayerStatusType;

/* ============================================
 * STRUKTURA PARSOVANE ZPRAVY
 * ============================================ */

#define MAX_PARAMS 10
#define MAX_PARAM_LENGTH 128

typedef struct {
    MessageType type;
    int param_count;
    char params[MAX_PARAMS][MAX_PARAM_LENGTH];
    char raw[512];  /* Puvodni zprava */
} ParsedMessage;

/* ============================================
 * FUNKCE PRO PRACI S PROTOKOLEM
 * ============================================ */

/**
 * Parsuje prijatou zpravu
 * @param raw_message Surova zprava (bez \n)
 * @param parsed Vystupni struktura
 * @return true pokud se podarilo parsovat, false jinak
 */
bool protocol_parse_message(const char *raw_message, ParsedMessage *parsed);

/**
 * Prevede typ zpravy na retezec
 * @param type Typ zpravy
 * @return Retezec s nazvem prikazu
 */
const char* protocol_message_type_to_string(MessageType type);

/**
 * Prevede retezec na typ zpravy
 * @param str Retezec s nazvem prikazu
 * @return Typ zpravy nebo MSG_UNKNOWN
 */
MessageType protocol_string_to_message_type(const char *str);

/**
 * Vytvori zpravu LOGIN_OK
 */
int protocol_create_login_ok(char *buffer, int size);

/**
 * Vytvori zpravu LOGIN_ERR
 */
int protocol_create_login_err(char *buffer, int size, ErrorCode code, const char *reason);

/**
 * Vytvori zpravu ROOMS
 */
int protocol_create_rooms(char *buffer, int size, const char *rooms_data);

/**
 * Vytvori zpravu ROOM_CREATED
 */
int protocol_create_room_created(char *buffer, int size, int room_id);

/**
 * Vytvori zpravu ROOM_JOINED
 */
int protocol_create_room_joined(char *buffer, int size, int room_id, const char *opponent);

/**
 * Vytvori zpravu ROOM_ERR
 */
int protocol_create_room_err(char *buffer, int size, ErrorCode code, const char *reason);

/**
 * Vytvori zpravu LEAVE_OK
 */
int protocol_create_leave_ok(char *buffer, int size);

/**
 * Vytvori zpravu GAME_START
 */
int protocol_create_game_start(char *buffer, int size, int stones, bool your_turn, const char *opponent);

/**
 * Vytvori zpravu TAKE_OK
 */
int protocol_create_take_ok(char *buffer, int size, int remaining, bool your_turn);

/**
 * Vytvori zpravu TAKE_ERR
 */
int protocol_create_take_err(char *buffer, int size, ErrorCode code, const char *reason);

/**
 * Vytvori zpravu SKIP_OK
 */
int protocol_create_skip_ok(char *buffer, int size, bool your_turn);

/**
 * Vytvori zpravu SKIP_ERR
 */
int protocol_create_skip_err(char *buffer, int size, ErrorCode code, const char *reason);

/**
 * Vytvori zpravu OPPONENT_ACTION
 */
int protocol_create_opponent_action(char *buffer, int size, const char *action, int param, int remaining);

/**
 * Vytvori zpravu GAME_OVER
 */
int protocol_create_game_over(char *buffer, int size, const char *winner, const char *loser);

/**
 * Vytvori zpravu PONG
 */
int protocol_create_pong(char *buffer, int size);

/**
 * Vytvori zpravu PLAYER_STATUS
 */
int protocol_create_player_status(char *buffer, int size, const char *nickname, PlayerStatusType status);

/**
 * Vytvori zpravu ERROR
 */
int protocol_create_error(char *buffer, int size, ErrorCode code, const char *message);

/**
 * Vytvori zpravu SERVER_SHUTDOWN
 */
int protocol_create_server_shutdown(char *buffer, int size);

/**
 * Vytvori zpravu WAIT_OPPONENT
 */
int protocol_create_wait_opponent(char *buffer, int size);

/**
 * Vytvori zpravu GAME_RESUMED
 */
int protocol_create_game_resumed(char *buffer, int size, int stones, bool your_turn, 
                                  int your_skips, int opponent_skips);

/**
 * Prevede chybovy kod na textovy popis
 */
const char* protocol_error_to_string(ErrorCode code);

/**
 * Validuje prezdivku
 * @return ERR_NONE pokud je validni, jinak prislusny chybovy kod
 */
ErrorCode protocol_validate_nickname(const char *nickname);

/**
 * Validuje nazev mistnosti
 * @return ERR_NONE pokud je validni, jinak prislusny chybovy kod
 */
ErrorCode protocol_validate_room_name(const char *name);

#endif /* PROTOCOL_H */

