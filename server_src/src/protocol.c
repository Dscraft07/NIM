/**
 * @file protocol.c
 * @brief Implementace protokolu pro komunikaci klient-server
 */

#include "protocol.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================
 * MAPOVANI ZPRAV NA RETEZCE
 * ============================================ */

static const struct {
    MessageType type;
    const char *name;
} message_map[] = {
    { MSG_LOGIN,          "LOGIN" },
    { MSG_LIST_ROOMS,     "LIST_ROOMS" },
    { MSG_CREATE_ROOM,    "CREATE_ROOM" },
    { MSG_JOIN_ROOM,      "JOIN_ROOM" },
    { MSG_LEAVE_ROOM,     "LEAVE_ROOM" },
    { MSG_TAKE,           "TAKE" },
    { MSG_SKIP,           "SKIP" },
    { MSG_PING,           "PING" },
    { MSG_LOGOUT,         "LOGOUT" },
    { MSG_LOGIN_OK,       "LOGIN_OK" },
    { MSG_LOGIN_ERR,      "LOGIN_ERR" },
    { MSG_ROOMS,          "ROOMS" },
    { MSG_ROOM_CREATED,   "ROOM_CREATED" },
    { MSG_ROOM_JOINED,    "ROOM_JOINED" },
    { MSG_ROOM_ERR,       "ROOM_ERR" },
    { MSG_LEAVE_OK,       "LEAVE_OK" },
    { MSG_GAME_START,     "GAME_START" },
    { MSG_TAKE_OK,        "TAKE_OK" },
    { MSG_TAKE_ERR,       "TAKE_ERR" },
    { MSG_SKIP_OK,        "SKIP_OK" },
    { MSG_SKIP_ERR,       "SKIP_ERR" },
    { MSG_OPPONENT_ACTION,"OPPONENT_ACTION" },
    { MSG_GAME_OVER,      "GAME_OVER" },
    { MSG_PONG,           "PONG" },
    { MSG_PLAYER_STATUS,  "PLAYER_STATUS" },
    { MSG_ERROR,          "ERROR" },
    { MSG_SERVER_SHUTDOWN,"SERVER_SHUTDOWN" },
    { MSG_WAIT_OPPONENT,  "WAIT_OPPONENT" },
    { MSG_GAME_RESUMED,   "GAME_RESUMED" },
    { MSG_UNKNOWN,        NULL }
};

/* ============================================
 * MAPOVANI CHYBOVYCH KODU
 * ============================================ */

static const struct {
    ErrorCode code;
    const char *message;
} error_map[] = {
    { ERR_NONE,             "OK" },
    { ERR_INVALID_FORMAT,   "Invalid message format" },
    { ERR_UNKNOWN_COMMAND,  "Unknown command" },
    { ERR_INVALID_PARAMS,   "Invalid parameters" },
    { ERR_NOT_LOGGED_IN,    "Not logged in" },
    { ERR_ALREADY_LOGGED_IN,"Already logged in" },
    { ERR_NICKNAME_TAKEN,   "Nickname already taken" },
    { ERR_NICKNAME_INVALID, "Invalid nickname" },
    { ERR_ROOM_NOT_FOUND,   "Room not found" },
    { ERR_ROOM_FULL,        "Room is full" },
    { ERR_ROOM_NAME_TAKEN,  "Room name already taken" },
    { ERR_NOT_IN_ROOM,      "Not in a room" },
    { ERR_NOT_IN_GAME,      "Not in a game" },
    { ERR_NOT_YOUR_TURN,    "Not your turn" },
    { ERR_INVALID_MOVE,     "Invalid move" },
    { ERR_NO_SKIPS_LEFT,    "No skips remaining" },
    { ERR_SERVER_FULL,      "Server is full" },
    { ERR_MAX_ROOMS,        "Maximum rooms reached" },
    { ERR_GAME_IN_PROGRESS, "Game already in progress" },
    { ERR_INTERNAL,         "Internal server error" }
};

/* ============================================
 * POMOCNE FUNKCE
 * ============================================ */

/**
 * Bezpecne kopirovani retezce
 */
static void safe_strcpy(char *dest, const char *src, size_t size) {
    if (size > 0) {
        strncpy(dest, src, size - 1);
        dest[size - 1] = '\0';
    }
}

/* ============================================
 * IMPLEMENTACE VEREJNYCH FUNKCI
 * ============================================ */

const char* protocol_message_type_to_string(MessageType type) {
    for (int i = 0; message_map[i].name != NULL; i++) {
        if (message_map[i].type == type) {
            return message_map[i].name;
        }
    }
    return "UNKNOWN";
}

MessageType protocol_string_to_message_type(const char *str) {
    if (str == NULL) return MSG_UNKNOWN;
    
    for (int i = 0; message_map[i].name != NULL; i++) {
        if (strcmp(message_map[i].name, str) == 0) {
            return message_map[i].type;
        }
    }
    return MSG_UNKNOWN;
}

bool protocol_parse_message(const char *raw_message, ParsedMessage *parsed) {
    if (raw_message == NULL || parsed == NULL) {
        return false;
    }
    
    /* Inicializace */
    memset(parsed, 0, sizeof(ParsedMessage));
    safe_strcpy(parsed->raw, raw_message, sizeof(parsed->raw));
    
    /* Kontrola delky */
    size_t len = strlen(raw_message);
    if (len == 0 || len >= MAX_MESSAGE_LENGTH) {
        parsed->type = MSG_UNKNOWN;
        return false;
    }
    
    /* Kopie pro parsovani */
    char buffer[MAX_MESSAGE_LENGTH];
    safe_strcpy(buffer, raw_message, sizeof(buffer));
    
    /* Odstraneni koncoveho \n nebo \r\n */
    char *newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';
    newline = strchr(buffer, '\r');
    if (newline) *newline = '\0';
    
    /* Rozdeleni na tokeny podle MSG_DELIMITER */
    char *token;
    char *saveptr;
    int param_index = 0;
    
    token = strtok_r(buffer, ";", &saveptr);
    if (token == NULL) {
        parsed->type = MSG_UNKNOWN;
        return false;
    }
    
    /* Prvni token je prikaz */
    parsed->type = protocol_string_to_message_type(token);
    
    /* Ostatni tokeny jsou parametry */
    while ((token = strtok_r(NULL, ";", &saveptr)) != NULL && 
           param_index < MAX_PARAMS) {
        safe_strcpy(parsed->params[param_index], token, MAX_PARAM_LENGTH);
        param_index++;
    }
    parsed->param_count = param_index;
    
    return (parsed->type != MSG_UNKNOWN);
}

const char* protocol_error_to_string(ErrorCode code) {
    for (size_t i = 0; i < sizeof(error_map) / sizeof(error_map[0]); i++) {
        if (error_map[i].code == code) {
            return error_map[i].message;
        }
    }
    return "Unknown error";
}

ErrorCode protocol_validate_nickname(const char *nickname) {
    if (nickname == NULL || strlen(nickname) == 0) {
        return ERR_NICKNAME_INVALID;
    }
    
    size_t len = strlen(nickname);
    if (len > MAX_NICKNAME_LENGTH) {
        return ERR_NICKNAME_INVALID;
    }
    
    /* Kontrola povolenych znaku (alfanumericke + podtrzitko) */
    for (size_t i = 0; i < len; i++) {
        char c = nickname[i];
        if (!isalnum(c) && c != '_') {
            return ERR_NICKNAME_INVALID;
        }
    }
    
    /* Prvni znak musi byt pismeno */
    if (!isalpha(nickname[0])) {
        return ERR_NICKNAME_INVALID;
    }
    
    return ERR_NONE;
}

ErrorCode protocol_validate_room_name(const char *name) {
    if (name == NULL || strlen(name) == 0) {
        return ERR_INVALID_PARAMS;
    }
    
    size_t len = strlen(name);
    if (len > MAX_ROOM_NAME_LENGTH) {
        return ERR_INVALID_PARAMS;
    }
    
    /* Kontrola povolenych znaku (alfanumericke + podtrzitko + mezera) */
    for (size_t i = 0; i < len; i++) {
        char c = name[i];
        if (!isalnum(c) && c != '_' && c != ' ') {
            return ERR_INVALID_PARAMS;
        }
    }
    
    return ERR_NONE;
}

/* ============================================
 * FUNKCE PRO TVORBU ZPRAV
 * ============================================ */

int protocol_create_login_ok(char *buffer, int size) {
    return snprintf(buffer, size, "LOGIN_OK\n");
}

int protocol_create_login_err(char *buffer, int size, ErrorCode code, const char *reason) {
    return snprintf(buffer, size, "LOGIN_ERR;%d;%s\n", code, 
                    reason ? reason : protocol_error_to_string(code));
}

int protocol_create_rooms(char *buffer, int size, const char *rooms_data) {
    if (rooms_data && strlen(rooms_data) > 0) {
        return snprintf(buffer, size, "ROOMS;%s\n", rooms_data);
    }
    return snprintf(buffer, size, "ROOMS;0\n");
}

int protocol_create_room_created(char *buffer, int size, int room_id) {
    return snprintf(buffer, size, "ROOM_CREATED;%d\n", room_id);
}

int protocol_create_room_joined(char *buffer, int size, int room_id, const char *opponent) {
    if (opponent && strlen(opponent) > 0) {
        return snprintf(buffer, size, "ROOM_JOINED;%d;%s\n", room_id, opponent);
    }
    return snprintf(buffer, size, "ROOM_JOINED;%d;\n", room_id);
}

int protocol_create_room_err(char *buffer, int size, ErrorCode code, const char *reason) {
    return snprintf(buffer, size, "ROOM_ERR;%d;%s\n", code,
                    reason ? reason : protocol_error_to_string(code));
}

int protocol_create_leave_ok(char *buffer, int size) {
    return snprintf(buffer, size, "LEAVE_OK\n");
}

int protocol_create_game_start(char *buffer, int size, int stones, bool your_turn, const char *opponent) {
    return snprintf(buffer, size, "GAME_START;%d;%d;%s\n", 
                    stones, your_turn ? 1 : 0, opponent ? opponent : "");
}

int protocol_create_take_ok(char *buffer, int size, int remaining, bool your_turn) {
    return snprintf(buffer, size, "TAKE_OK;%d;%d\n", remaining, your_turn ? 1 : 0);
}

int protocol_create_take_err(char *buffer, int size, ErrorCode code, const char *reason) {
    return snprintf(buffer, size, "TAKE_ERR;%d;%s\n", code,
                    reason ? reason : protocol_error_to_string(code));
}

int protocol_create_skip_ok(char *buffer, int size, bool your_turn) {
    return snprintf(buffer, size, "SKIP_OK;%d\n", your_turn ? 1 : 0);
}

int protocol_create_skip_err(char *buffer, int size, ErrorCode code, const char *reason) {
    return snprintf(buffer, size, "SKIP_ERR;%d;%s\n", code,
                    reason ? reason : protocol_error_to_string(code));
}

int protocol_create_opponent_action(char *buffer, int size, const char *action, int param, int remaining) {
    return snprintf(buffer, size, "OPPONENT_ACTION;%s;%d;%d\n", action, param, remaining);
}

int protocol_create_game_over(char *buffer, int size, const char *winner, const char *loser) {
    return snprintf(buffer, size, "GAME_OVER;%s;%s\n", winner, loser);
}

int protocol_create_pong(char *buffer, int size) {
    return snprintf(buffer, size, "PONG\n");
}

int protocol_create_player_status(char *buffer, int size, const char *nickname, PlayerStatusType status) {
    const char *status_str;
    switch (status) {
        case STATUS_CONNECTED:    status_str = "CONNECTED"; break;
        case STATUS_DISCONNECTED: status_str = "DISCONNECTED"; break;
        case STATUS_RECONNECTED:  status_str = "RECONNECTED"; break;
        default:                  status_str = "UNKNOWN"; break;
    }
    return snprintf(buffer, size, "PLAYER_STATUS;%s;%s\n", nickname, status_str);
}

int protocol_create_error(char *buffer, int size, ErrorCode code, const char *message) {
    return snprintf(buffer, size, "ERROR;%d;%s\n", code,
                    message ? message : protocol_error_to_string(code));
}

int protocol_create_server_shutdown(char *buffer, int size) {
    return snprintf(buffer, size, "SERVER_SHUTDOWN\n");
}

int protocol_create_wait_opponent(char *buffer, int size) {
    return snprintf(buffer, size, "WAIT_OPPONENT\n");
}

int protocol_create_game_resumed(char *buffer, int size, int stones, bool your_turn,
                                  int your_skips, int opponent_skips) {
    return snprintf(buffer, size, "GAME_RESUMED;%d;%d;%d;%d\n",
                    stones, your_turn ? 1 : 0, your_skips, opponent_skips);
}

