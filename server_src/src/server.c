/**
 * @file server.c
 * @brief Implementace hlavniho serveroveho modulu
 */

#include "server.h"
#include "protocol.h"
#include "logger.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <getopt.h>

/* ============================================
 * GLOBALNI PROMENNE
 * ============================================ */

static volatile sig_atomic_t g_shutdown_requested = 0;

/* ============================================
 * SIGNAL HANDLER
 * ============================================ */

static void signal_handler(int sig) {
    (void)sig;
    g_shutdown_requested = 1;
}

/* ============================================
 * POMOCNE FUNKCE
 * ============================================ */

/**
 * Nastavi socket jako non-blocking
 */
static bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

/**
 * Aktualizuje max_fd pro select
 */
static void update_max_fd(Server *server) {
    server->max_fd = server->listen_fd;
    
    for (int i = 0; i < server->config.max_clients; i++) {
        if (server->players[i].is_active && server->players[i].socket_fd > server->max_fd) {
            server->max_fd = server->players[i].socket_fd;
        }
    }
}

/**
 * Prijme noveho klienta
 */
static void accept_new_client(Server *server) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server->listen_fd, 
                           (struct sockaddr*)&client_addr, 
                           &client_len);
    
    if (client_fd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            LOG_ERROR("Accept failed: %s", strerror(errno));
        }
        return;
    }
    
    /* Nastav non-blocking */
    if (!set_nonblocking(client_fd)) {
        LOG_ERROR("Failed to set non-blocking for client socket");
        close(client_fd);
        return;
    }
    
    /* Nastav TCP keepalive pro detekci odpojeneho klienta */
    int keepalive = 1;
    setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
    
#ifdef TCP_KEEPIDLE
    int keepidle = 10;  /* Zacni keepalive po 10s neaktivity */
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
#endif

#ifdef TCP_KEEPINTVL
    int keepintvl = 5;  /* Interval mezi keepalive proby */
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
#endif

#ifdef TCP_KEEPCNT
    int keepcnt = 3;    /* Pocet pokusu pred odpojenim */
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
#endif
    
    /* Najdi volny slot */
    int slot = player_find_free_slot(server->players, server->config.max_clients);
    if (slot < 0) {
        LOG_WARNING("Server full, rejecting connection from %s", 
                    inet_ntoa(client_addr.sin_addr));
        /* Posli chybu a zavri */
        char buffer[128];
        protocol_create_login_err(buffer, sizeof(buffer), ERR_SERVER_FULL, NULL);
        send(client_fd, buffer, strlen(buffer), 0);
        close(client_fd);
        return;
    }
    
    /* Vytvor hrace */
    player_create(&server->players[slot], client_fd);
    
    LOG_INFO("New client connected from %s:%d (slot %d, fd %d)",
             inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port),
             slot, client_fd);
    
    update_max_fd(server);
}

/**
 * Zkontroluje, zda data obsahuji pouze tisknutelne znaky
 * Povoli: tisknutelne ASCII (32-126), \n, \r, ;
 * @return true pokud jsou data validni
 */
static bool is_valid_protocol_data(const char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)data[i];
        /* Povol tisknutelne znaky, \n, \r */
        if (c >= 32 && c <= 126) continue;
        if (c == '\n' || c == '\r') continue;
        /* Cokoliv jineho je binarni/nevalidni */
        return false;
    }
    return true;
}

/**
 * Zkontroluje rate limit pro hrace
 * @return true pokud je v limitu
 */
static bool check_rate_limit(Player *player) {
    time_t now = time(NULL);
    
    if (player->rate_limit_second != now) {
        player->rate_limit_second = now;
        player->messages_this_second = 0;
    }
    
    player->messages_this_second++;
    
    if (player->messages_this_second > MAX_MESSAGES_PER_SECOND) {
        return false;
    }
    
    return true;
}

/**
 * Precte data od klienta
 */
static void read_from_client(Server *server, Player *player) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    bytes_read = recv(player->socket_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            LOG_INFO("Client '%s' disconnected (connection closed)",
                     player->nickname[0] ? player->nickname : "(unknown)");
        } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
            LOG_WARNING("Read error from '%s': %s",
                        player->nickname[0] ? player->nickname : "(unknown)",
                        strerror(errno));
        }
        server_handle_disconnect(server, player, false);
        return;
    }
    
    buffer[bytes_read] = '\0';
    player_update_activity(player);
    
    /* OCHRANA: Zkontroluj, zda data obsahuji pouze validni znaky */
    if (!is_valid_protocol_data(buffer, bytes_read)) {
        LOG_WARNING("Binary/invalid data from '%s', counting as invalid message",
                    player->nickname[0] ? player->nickname : "(unknown)");
        player->invalid_message_count++;
        
        if (player->invalid_message_count >= MAX_INVALID_MESSAGES) {
            LOG_WARNING("Too many invalid messages from '%s', disconnecting",
                        player->nickname[0] ? player->nickname : "(unknown)");
            char response[BUFFER_SIZE];
            protocol_create_error(response, sizeof(response), ERR_INVALID_FORMAT,
                                  "Binary data not allowed");
            server_send_to_player(player, response);
            server_handle_disconnect(server, player, false);
        }
        /* Zahodit binarni data - nevkladat do bufferu */
        return;
    }
    
    /* Pridej do bufferu hrace */
    int space_left = BUFFER_SIZE - player->recv_buffer_len - 1;
    if ((int)bytes_read > space_left) {
        LOG_WARNING("Buffer overflow for player '%s', disconnecting",
                    player->nickname[0] ? player->nickname : "(unknown)");
        server_handle_disconnect(server, player, false);
        return;
    }
    
    memcpy(player->recv_buffer + player->recv_buffer_len, buffer, bytes_read);
    player->recv_buffer_len += bytes_read;
    player->recv_buffer[player->recv_buffer_len] = '\0';
    
    /* OCHRANA: Kontrola proti flood bez newline */
    if (player->recv_buffer_len > MAX_MESSAGE_WITHOUT_NEWLINE && 
        strchr(player->recv_buffer, '\n') == NULL) {
        LOG_WARNING("Message too long without newline from '%s', disconnecting",
                    player->nickname[0] ? player->nickname : "(unknown)");
        char response[BUFFER_SIZE];
        protocol_create_error(response, sizeof(response), ERR_INVALID_FORMAT,
                              "Message too long");
        server_send_to_player(player, response);
        server_handle_disconnect(server, player, false);
        return;
    }
    
    /* Zpracuj vsechny kompletni zpravy (oddelene \n) */
    char *line_start = player->recv_buffer;
    char *newline;
    
    while ((newline = strchr(line_start, '\n')) != NULL) {
        *newline = '\0';
        
        /* Odstran pripadny \r */
        if (newline > line_start && *(newline - 1) == '\r') {
            *(newline - 1) = '\0';
        }
        
        size_t msg_len = strlen(line_start);
        if (msg_len > 0) {
            /* OCHRANA: Rate limiting */
            if (!check_rate_limit(player)) {
                LOG_WARNING("Rate limit exceeded for '%s'",
                            player->nickname[0] ? player->nickname : "(unknown)");
                player->invalid_message_count++;
                /* Preskoc tuto zpravu, ale pokracuj */
            } else {
                LOG_DEBUG("Received from '%s': %s",
                          player->nickname[0] ? player->nickname : "(unknown)",
                          line_start);
                server_handle_message(server, player, line_start);
                
                /* Po zpracovani zkontroluj, zda hrac nebyl odpojen */
                if (player->socket_fd < 0 || !player->is_active) {
                    /* Hrac byl odpojen behem zpracovani zpravy - ukoncit */
                    return;
                }
            }
        }
        
        line_start = newline + 1;
    }
    
    /* Presun zbytek bufferu na zacatek */
    int remaining = player->recv_buffer_len - (line_start - player->recv_buffer);
    if (remaining > 0 && remaining <= BUFFER_SIZE) {
        memmove(player->recv_buffer, line_start, remaining);
        player->recv_buffer_len = remaining;
        player->recv_buffer[remaining] = '\0';
    } else {
        /* Neplatna hodnota remaining - reset bufferu */
        player->recv_buffer_len = 0;
        player->recv_buffer[0] = '\0';
    }
}

/* ============================================
 * ZPRACOVANI ZPRAV
 * ============================================ */

/**
 * Zpracuje LOGIN
 */
static void handle_login(Server *server, Player *player, ParsedMessage *msg) {
    char response[BUFFER_SIZE];
    
    /* Kontrola stavu */
    if (player->state != PLAYER_STATE_CONNECTING) {
        protocol_create_login_err(response, sizeof(response), 
                                  ERR_ALREADY_LOGGED_IN, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    /* Kontrola parametru */
    if (msg->param_count < 1) {
        protocol_create_login_err(response, sizeof(response), 
                                  ERR_INVALID_PARAMS, "Missing nickname");
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    const char *nickname = msg->params[0];
    
    /* Validace prezdivky */
    ErrorCode err = protocol_validate_nickname(nickname);
    if (err != ERR_NONE) {
        protocol_create_login_err(response, sizeof(response), err, NULL);
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    /* Kontrola, zda existuje odpojeny hrac se stejnou prezdivkou (reconnect) */
    Player *disconnected = player_find_disconnected(server->players, 
                                                    server->config.max_clients, 
                                                    nickname);
    if (disconnected != NULL) {
        /* Reconnect - prevezmi session */
        LOG_INFO("Player '%s' reconnecting", nickname);
        
        /* Zkopiruj stav z odpojeneho hrace */
        int old_room_id = disconnected->room_id;
        int old_skips = disconnected->skips_remaining;
        PlayerState old_state = disconnected->state == PLAYER_STATE_DISCONNECTED ? 
                                PLAYER_STATE_IN_GAME : disconnected->state;
        
        /* Resetuj stareho hrace */
        player_reset(disconnected, false);
        
        /* Nastav noveho hrace */
        player_set_nickname(player, nickname);
        player->room_id = old_room_id;
        player->skips_remaining = old_skips;
        
        /* Posli potvrzeni */
        protocol_create_login_ok(response, sizeof(response));
        server_send_to_player(player, response);
        
        /* Obnov do mistnosti, pokud byl ve hre */
        if (old_room_id >= 0) {
            Room *room = room_find_by_id(server->rooms, server->config.max_rooms, old_room_id);
            if (room != NULL) {
                /* Aktualizuj ukazatel v mistnosti */
                for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
                    if (room->players[i] == disconnected) {
                        room->players[i] = player;
                        break;
                    }
                }
                
                player_set_state(player, old_state);
                
                /* Obnov hru, pokud byla pozastavena */
                if (room->game.state == GAME_STATE_PAUSED) {
                    game_resume(&room->game);
                    
                    /* Posli stav hry */
                    int player_idx = room_get_player_index(room, player);
                    bool my_turn = game_is_player_turn(&room->game, player_idx);
                    int opp_idx = 1 - player_idx;
                    
                    protocol_create_game_resumed(response, sizeof(response),
                                                  game_get_stones(&room->game),
                                                  my_turn,
                                                  player->skips_remaining,
                                                  room->game.player_skips[opp_idx]);
                    server_send_to_player(player, response);
                    
                    /* Informuj protihrace */
                    Player *opponent = room_get_opponent(room, player);
                    if (opponent != NULL && opponent->socket_fd >= 0) {
                        protocol_create_player_status(response, sizeof(response),
                                                      nickname, STATUS_RECONNECTED);
                        server_send_to_player(opponent, response);
                    }
                }
            }
        } else {
            player_set_state(player, PLAYER_STATE_LOBBY);
        }
        
        return;
    }
    
    /* Kontrola, zda prezdivka neni obsazena */
    Player *existing = player_find_by_nickname(server->players, 
                                               server->config.max_clients, 
                                               nickname);
    if (existing != NULL) {
        protocol_create_login_err(response, sizeof(response), 
                                  ERR_NICKNAME_TAKEN, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    /* Uspesny login */
    player_set_nickname(player, nickname);
    player_set_state(player, PLAYER_STATE_LOBBY);
    
    protocol_create_login_ok(response, sizeof(response));
    server_send_to_player(player, response);
    
    LOG_INFO("Player '%s' logged in", nickname);
}

/**
 * Zpracuje LIST_ROOMS
 */
static void handle_list_rooms(Server *server, Player *player, ParsedMessage *msg) {
    (void)msg;
    char response[BUFFER_SIZE];
    char rooms_data[BUFFER_SIZE - 64];
    
    if (player->state == PLAYER_STATE_CONNECTING) {
        protocol_create_error(response, sizeof(response), ERR_NOT_LOGGED_IN, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    room_list_to_string(server->rooms, server->config.max_rooms, 
                        rooms_data, sizeof(rooms_data));
    protocol_create_rooms(response, sizeof(response), rooms_data);
    server_send_to_player(player, response);
}

/**
 * Zpracuje CREATE_ROOM
 */
static void handle_create_room(Server *server, Player *player, ParsedMessage *msg) {
    char response[BUFFER_SIZE];
    
    if (player->state != PLAYER_STATE_LOBBY) {
        ErrorCode err = (player->state == PLAYER_STATE_CONNECTING) ? 
                        ERR_NOT_LOGGED_IN : ERR_GAME_IN_PROGRESS;
        protocol_create_room_err(response, sizeof(response), err, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    if (msg->param_count < 1) {
        protocol_create_room_err(response, sizeof(response), 
                                 ERR_INVALID_PARAMS, "Missing room name");
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    const char *room_name = msg->params[0];
    
    /* Validace nazvu */
    ErrorCode err = protocol_validate_room_name(room_name);
    if (err != ERR_NONE) {
        protocol_create_room_err(response, sizeof(response), err, NULL);
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    /* Kontrola limitu mistnosti */
    if (room_count_active(server->rooms, server->config.max_rooms) >= server->config.max_rooms) {
        protocol_create_room_err(response, sizeof(response), ERR_MAX_ROOMS, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    /* Vytvor mistnost */
    int room_id = room_create(server->rooms, server->config.max_rooms, room_name, player);
    if (room_id < 0) {
        /* Nazev obsazen nebo jina chyba */
        protocol_create_room_err(response, sizeof(response), ERR_ROOM_NAME_TAKEN, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    player_set_state(player, PLAYER_STATE_IN_ROOM);
    
    protocol_create_room_created(response, sizeof(response), room_id);
    server_send_to_player(player, response);
    
    /* Posli WAIT_OPPONENT */
    protocol_create_wait_opponent(response, sizeof(response));
    server_send_to_player(player, response);
}

/**
 * Zpracuje JOIN_ROOM
 */
static void handle_join_room(Server *server, Player *player, ParsedMessage *msg) {
    char response[BUFFER_SIZE];
    
    if (player->state != PLAYER_STATE_LOBBY) {
        ErrorCode err = (player->state == PLAYER_STATE_CONNECTING) ? 
                        ERR_NOT_LOGGED_IN : ERR_GAME_IN_PROGRESS;
        protocol_create_room_err(response, sizeof(response), err, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    if (msg->param_count < 1) {
        protocol_create_room_err(response, sizeof(response), 
                                 ERR_INVALID_PARAMS, "Missing room ID");
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    int room_id = atoi(msg->params[0]);
    Room *room = room_find_by_id(server->rooms, server->config.max_rooms, room_id);
    
    if (room == NULL) {
        protocol_create_room_err(response, sizeof(response), ERR_ROOM_NOT_FOUND, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    if (room_is_full(room)) {
        protocol_create_room_err(response, sizeof(response), ERR_ROOM_FULL, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    /* Pridej hrace */
    Player *opponent = room_get_opponent(room, NULL); /* Prvni hrac v mistnosti */
    
    if (!room_add_player(room, player)) {
        protocol_create_room_err(response, sizeof(response), ERR_INTERNAL, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    player_set_state(player, PLAYER_STATE_IN_ROOM);
    
    /* Posli potvrzeni s prezdivkou protihrace */
    protocol_create_room_joined(response, sizeof(response), room_id, 
                                 opponent ? opponent->nickname : "");
    server_send_to_player(player, response);
    
    /* Pokud je mistnost plna, zacni hru */
    if (room_is_full(room)) {
        room_start_game(room);
        
        /* Posli GAME_START obema hracum */
        for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
            Player *p = room->players[i];
            if (p != NULL && p->socket_fd >= 0) {
                Player *opp = room_get_opponent(room, p);
                bool my_turn = game_is_player_turn(&room->game, i);
                protocol_create_game_start(response, sizeof(response),
                                            game_get_stones(&room->game),
                                            my_turn,
                                            opp ? opp->nickname : "");
                server_send_to_player(p, response);
            }
        }
    }
}

/**
 * Zpracuje LEAVE_ROOM
 */
static void handle_leave_room(Server *server, Player *player, ParsedMessage *msg) {
    (void)msg;
    char response[BUFFER_SIZE];
    
    if (player->state != PLAYER_STATE_IN_ROOM && player->state != PLAYER_STATE_IN_GAME) {
        protocol_create_error(response, sizeof(response), ERR_NOT_IN_ROOM, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    Room *room = room_find_by_id(server->rooms, server->config.max_rooms, player->room_id);
    if (room == NULL) {
        protocol_create_error(response, sizeof(response), ERR_INTERNAL, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    Player *opponent = room_get_opponent(room, player);
    
    /* Pokud hra probihala, protihrac vyhrává */
    if (room->game.state == GAME_STATE_PLAYING || room->game.state == GAME_STATE_PAUSED) {
        room->game.state = GAME_STATE_FINISHED;
        
        if (opponent != NULL && opponent->socket_fd >= 0) {
            protocol_create_game_over(response, sizeof(response),
                                       opponent->nickname, player->nickname);
            server_send_to_player(opponent, response);
            
            /* Presun protihrace do lobby */
            room_remove_player(room, opponent);
            player_set_state(opponent, PLAYER_STATE_LOBBY);
        }
    } else if (opponent != NULL && opponent->socket_fd >= 0) {
        /* Informuj protihrace */
        protocol_create_player_status(response, sizeof(response),
                                       player->nickname, STATUS_DISCONNECTED);
        server_send_to_player(opponent, response);
        
        /* Presun protihrace do lobby */
        room_remove_player(room, opponent);
        player_set_state(opponent, PLAYER_STATE_LOBBY);
    }
    
    /* Odeber hrace z mistnosti */
    room_remove_player(room, player);
    player_set_state(player, PLAYER_STATE_LOBBY);
    
    protocol_create_leave_ok(response, sizeof(response));
    server_send_to_player(player, response);
}

/**
 * Zpracuje TAKE
 */
static void handle_take(Server *server, Player *player, ParsedMessage *msg) {
    char response[BUFFER_SIZE];
    
    if (player->state != PLAYER_STATE_IN_GAME) {
        protocol_create_take_err(response, sizeof(response), ERR_NOT_IN_GAME, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    if (msg->param_count < 1) {
        protocol_create_take_err(response, sizeof(response), 
                                 ERR_INVALID_PARAMS, "Missing count");
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    int count = atoi(msg->params[0]);
    
    Room *room = room_find_by_id(server->rooms, server->config.max_rooms, player->room_id);
    if (room == NULL) {
        protocol_create_take_err(response, sizeof(response), ERR_INTERNAL, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    int player_idx = room_get_player_index(room, player);
    
    /* Kontrola, zda je hrac na tahu */
    if (!game_is_player_turn(&room->game, player_idx)) {
        protocol_create_take_err(response, sizeof(response), ERR_NOT_YOUR_TURN, NULL);
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    /* Validace tahu */
    if (!game_validate_take_count(&room->game, count)) {
        protocol_create_take_err(response, sizeof(response), ERR_INVALID_MOVE, NULL);
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    /* Proved tah */
    if (!game_take_stones(&room->game, player_idx, count)) {
        protocol_create_take_err(response, sizeof(response), ERR_INVALID_MOVE, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    int remaining = game_get_stones(&room->game);
    Player *opponent = room_get_opponent(room, player);
    
    /* Kontrola konce hry */
    if (game_is_over(&room->game)) {
        int winner_idx = game_get_winner(&room->game);
        Player *winner = room->players[winner_idx];
        Player *loser = room->players[1 - winner_idx];
        
        /* Posli GAME_OVER obema */
        protocol_create_game_over(response, sizeof(response),
                                   winner->nickname, loser->nickname);
        server_send_to_player(player, response);
        
        if (opponent != NULL && opponent->socket_fd >= 0) {
            server_send_to_player(opponent, response);
        }
        
        /* Presun hrace do lobby */
        player_set_state(player, PLAYER_STATE_LOBBY);
        if (opponent != NULL) {
            player_set_state(opponent, PLAYER_STATE_LOBBY);
            room_remove_player(room, opponent);
        }
        room_remove_player(room, player);
        
        return;
    }
    
    /* Posli potvrzeni hraci */
    bool still_my_turn = game_is_player_turn(&room->game, player_idx);
    protocol_create_take_ok(response, sizeof(response), remaining, still_my_turn);
    server_send_to_player(player, response);
    
    /* Posli akci protihracovi */
    if (opponent != NULL && opponent->socket_fd >= 0) {
        protocol_create_opponent_action(response, sizeof(response), 
                                        "TAKE", count, remaining);
        server_send_to_player(opponent, response);
    }
}

/**
 * Zpracuje SKIP
 */
static void handle_skip(Server *server, Player *player, ParsedMessage *msg) {
    (void)msg;
    char response[BUFFER_SIZE];
    
    if (player->state != PLAYER_STATE_IN_GAME) {
        protocol_create_skip_err(response, sizeof(response), ERR_NOT_IN_GAME, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    Room *room = room_find_by_id(server->rooms, server->config.max_rooms, player->room_id);
    if (room == NULL) {
        protocol_create_skip_err(response, sizeof(response), ERR_INTERNAL, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    int player_idx = room_get_player_index(room, player);
    
    /* Kontrola, zda je hrac na tahu */
    if (!game_is_player_turn(&room->game, player_idx)) {
        protocol_create_skip_err(response, sizeof(response), ERR_NOT_YOUR_TURN, NULL);
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    /* Kontrola, zda ma preskoceni */
    if (!game_can_skip(&room->game, player_idx)) {
        protocol_create_skip_err(response, sizeof(response), ERR_NO_SKIPS_LEFT, NULL);
        server_send_to_player(player, response);
        player->invalid_message_count++;
        return;
    }
    
    /* Proved preskoceni */
    if (!game_skip_turn(&room->game, player_idx)) {
        protocol_create_skip_err(response, sizeof(response), ERR_INTERNAL, NULL);
        server_send_to_player(player, response);
        return;
    }
    
    player->skips_remaining = room->game.player_skips[player_idx];
    
    /* Posli potvrzeni */
    bool still_my_turn = game_is_player_turn(&room->game, player_idx);
    protocol_create_skip_ok(response, sizeof(response), still_my_turn);
    server_send_to_player(player, response);
    
    /* Informuj protihrace */
    Player *opponent = room_get_opponent(room, player);
    if (opponent != NULL && opponent->socket_fd >= 0) {
        protocol_create_opponent_action(response, sizeof(response), 
                                        "SKIP", 0, game_get_stones(&room->game));
        server_send_to_player(opponent, response);
    }
}

/**
 * Zpracuje PING
 */
static void handle_ping(Server *server, Player *player, ParsedMessage *msg) {
    (void)server;
    (void)msg;
    char response[BUFFER_SIZE];
    protocol_create_pong(response, sizeof(response));
    server_send_to_player(player, response);
}

/**
 * Zpracuje PONG (odpoved na nas PING)
 */
static void handle_pong(Server *server, Player *player, ParsedMessage *msg) {
    (void)server;
    (void)msg;
    player->waiting_pong = false;
    player_update_activity(player);
}

/**
 * Zpracuje LOGOUT
 */
static void handle_logout(Server *server, Player *player, ParsedMessage *msg) {
    (void)msg;
    LOG_INFO("Player '%s' logging out", 
             player->nickname[0] ? player->nickname : "(unknown)");
    server_handle_disconnect(server, player, true);
}

/* ============================================
 * IMPLEMENTACE VEREJNYCH FUNKCI
 * ============================================ */

bool server_init(Server *server, const ServerConfig *config) {
    if (server == NULL || config == NULL) return false;
    
    memset(server, 0, sizeof(Server));
    server->config = *config;
    server->running = false;
    server->listen_fd = -1;
    
    /* Alokace hracu */
    server->players = malloc(config->max_clients * sizeof(Player));
    if (server->players == NULL) {
        LOG_ERROR("Failed to allocate players array");
        return false;
    }
    player_init_all(server->players, config->max_clients);
    
    /* Alokace mistnosti */
    server->rooms = malloc(config->max_rooms * sizeof(Room));
    if (server->rooms == NULL) {
        LOG_ERROR("Failed to allocate rooms array");
        free(server->players);
        return false;
    }
    room_init_all(server->rooms, config->max_rooms);
    
    /* Vytvoreni socketu */
    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_fd < 0) {
        LOG_ERROR("Failed to create socket: %s", strerror(errno));
        free(server->players);
        free(server->rooms);
        return false;
    }
    
    /* SO_REUSEADDR */
    int optval = 1;
    setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    /* Non-blocking */
    if (!set_nonblocking(server->listen_fd)) {
        LOG_ERROR("Failed to set non-blocking: %s", strerror(errno));
        close(server->listen_fd);
        free(server->players);
        free(server->rooms);
        return false;
    }
    
    /* Bind */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config->port);
    
    if (inet_pton(AF_INET, config->bind_address, &addr.sin_addr) <= 0) {
        LOG_ERROR("Invalid bind address: %s", config->bind_address);
        close(server->listen_fd);
        free(server->players);
        free(server->rooms);
        return false;
    }
    
    if (bind(server->listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind to %s:%d: %s", 
                  config->bind_address, config->port, strerror(errno));
        close(server->listen_fd);
        free(server->players);
        free(server->rooms);
        return false;
    }
    
    /* Listen */
    if (listen(server->listen_fd, 10) < 0) {
        LOG_ERROR("Failed to listen: %s", strerror(errno));
        close(server->listen_fd);
        free(server->players);
        free(server->rooms);
        return false;
    }
    
    server->max_fd = server->listen_fd;
    
    LOG_INFO("Server initialized on %s:%d (max clients: %d, max rooms: %d)",
             config->bind_address, config->port, 
             config->max_clients, config->max_rooms);
    
    return true;
}

void server_run(Server *server) {
    if (server == NULL) return;
    
    /* Nastav signal handlery */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    server->running = true;
    LOG_INFO("Server started, waiting for connections...");
    
    while (server->running && !g_shutdown_requested) {
        /* Priprav fd_set */
        FD_ZERO(&server->read_fds);
        FD_SET(server->listen_fd, &server->read_fds);
        
        for (int i = 0; i < server->config.max_clients; i++) {
            if (server->players[i].is_active && server->players[i].socket_fd >= 0) {
                FD_SET(server->players[i].socket_fd, &server->read_fds);
            }
        }
        
        /* Select s timeoutem */
        struct timeval timeout;
        timeout.tv_sec = SELECT_TIMEOUT_SEC;
        timeout.tv_usec = SELECT_TIMEOUT_USEC;
        
        int activity = select(server->max_fd + 1, &server->read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (errno == EINTR) continue; /* Preruseno signalem */
            LOG_ERROR("Select error: %s", strerror(errno));
            break;
        }
        
        /* Nova spojeni */
        if (FD_ISSET(server->listen_fd, &server->read_fds)) {
            accept_new_client(server);
        }
        
        /* Data od klientu */
        for (int i = 0; i < server->config.max_clients; i++) {
            Player *player = &server->players[i];
            if (player->is_active && player->socket_fd >= 0 &&
                FD_ISSET(player->socket_fd, &server->read_fds)) {
                read_from_client(server, player);
            }
        }
        
        /* Kontrola timeoutu */
        server_check_timeouts(server);
    }
    
    LOG_INFO("Server shutting down...");
    server_shutdown(server);
}

void server_shutdown(Server *server) {
    if (server == NULL) return;
    
    server->running = false;
    
    /* Informuj vsechny klienty */
    char buffer[64];
    protocol_create_server_shutdown(buffer, sizeof(buffer));
    
    for (int i = 0; i < server->config.max_clients; i++) {
        if (server->players[i].is_active && server->players[i].socket_fd >= 0) {
            send(server->players[i].socket_fd, buffer, strlen(buffer), MSG_NOSIGNAL);
            close(server->players[i].socket_fd);
        }
    }
    
    /* Uvolni zdroje */
    if (server->listen_fd >= 0) {
        close(server->listen_fd);
    }
    
    free(server->players);
    free(server->rooms);
    
    LOG_INFO("Server shutdown complete");
}

bool server_send_to_player(Player *player, const char *message) {
    if (player == NULL || message == NULL || player->socket_fd < 0) {
        return false;
    }
    
    size_t len = strlen(message);
    ssize_t sent = send(player->socket_fd, message, len, MSG_NOSIGNAL);
    
    if (sent < 0) {
        LOG_WARNING("Failed to send to '%s': %s", 
                    player->nickname[0] ? player->nickname : "(unknown)",
                    strerror(errno));
        return false;
    }
    
    LOG_DEBUG("Sent to '%s': %.*s", 
              player->nickname[0] ? player->nickname : "(unknown)",
              (int)(len - 1), message); /* -1 to skip \n */
    
    return true;
}

void server_broadcast_to_room(Room *room, const char *message, Player *except) {
    if (room == NULL || message == NULL) return;
    
    for (int i = 0; i < PLAYERS_PER_ROOM; i++) {
        Player *player = room->players[i];
        if (player != NULL && player != except && player->socket_fd >= 0) {
            server_send_to_player(player, message);
        }
    }
}

void server_broadcast_to_lobby(Server *server, const char *message) {
    if (server == NULL || message == NULL) return;
    
    for (int i = 0; i < server->config.max_clients; i++) {
        Player *player = &server->players[i];
        if (player->is_active && player->state == PLAYER_STATE_LOBBY &&
            player->socket_fd >= 0) {
            server_send_to_player(player, message);
        }
    }
}

void server_handle_message(Server *server, Player *player, const char *message) {
    ParsedMessage parsed;
    
    if (!protocol_parse_message(message, &parsed)) {
        LOG_WARNING("Invalid message from '%s': %s",
                    player->nickname[0] ? player->nickname : "(unknown)",
                    message);
        player->invalid_message_count++;
        
        /* Kontrola limitu nevalidnich zprav */
        if (player->invalid_message_count >= MAX_INVALID_MESSAGES) {
            LOG_WARNING("Too many invalid messages from '%s', disconnecting",
                        player->nickname[0] ? player->nickname : "(unknown)");
            char response[BUFFER_SIZE];
            protocol_create_error(response, sizeof(response), ERR_INVALID_FORMAT,
                                  "Too many invalid messages");
            server_send_to_player(player, response);
            server_handle_disconnect(server, player, false);
        }
        return;
    }
    
    /* Dispatch podle typu zpravy */
    switch (parsed.type) {
        case MSG_LOGIN:
            handle_login(server, player, &parsed);
            break;
        case MSG_LIST_ROOMS:
            handle_list_rooms(server, player, &parsed);
            break;
        case MSG_CREATE_ROOM:
            handle_create_room(server, player, &parsed);
            break;
        case MSG_JOIN_ROOM:
            handle_join_room(server, player, &parsed);
            break;
        case MSG_LEAVE_ROOM:
            handle_leave_room(server, player, &parsed);
            break;
        case MSG_TAKE:
            handle_take(server, player, &parsed);
            break;
        case MSG_SKIP:
            handle_skip(server, player, &parsed);
            break;
        case MSG_PING:
            handle_ping(server, player, &parsed);
            break;
        case MSG_PONG:
            handle_pong(server, player, &parsed);
            break;
        case MSG_LOGOUT:
            handle_logout(server, player, &parsed);
            break;
        default:
            LOG_WARNING("Unknown message type from '%s': %s",
                        player->nickname[0] ? player->nickname : "(unknown)",
                        message);
            player->invalid_message_count++;
            break;
    }
}

void server_handle_disconnect(Server *server, Player *player, bool graceful) {
    if (server == NULL) {
        LOG_ERROR("server_handle_disconnect: server is NULL!");
        return;
    }
    if (player == NULL) {
        LOG_ERROR("server_handle_disconnect: player is NULL!");
        return;
    }
    
    LOG_DEBUG("server_handle_disconnect: player='%s', graceful=%d, room_id=%d",
              player->nickname[0] ? player->nickname : "(unknown)",
              graceful, player->room_id);
    
    char response[BUFFER_SIZE];
    
    /* Pokud je ve hre, informuj protihrace */
    if (player->room_id >= 0) {
        LOG_DEBUG("server_handle_disconnect: finding room %d", player->room_id);
        Room *room = room_find_by_id(server->rooms, server->config.max_rooms, player->room_id);
        
        if (room != NULL) {
            LOG_DEBUG("server_handle_disconnect: room found, getting opponent");
            Player *opponent = room_get_opponent(room, player);
            LOG_DEBUG("server_handle_disconnect: opponent=%p", (void*)opponent);
            
            if (graceful || opponent == NULL) {
                LOG_DEBUG("server_handle_disconnect: graceful path");
                /* Graceful disconnect nebo zadny protihrac - ukonci hru */
                if (room->game.state == GAME_STATE_PLAYING) {
                    room->game.state = GAME_STATE_FINISHED;
                    
                    if (opponent != NULL && opponent->socket_fd >= 0) {
                        protocol_create_game_over(response, sizeof(response),
                                                   opponent->nickname, player->nickname);
                        server_send_to_player(opponent, response);
                        
                        room_remove_player(room, opponent);
                        player_set_state(opponent, PLAYER_STATE_LOBBY);
                    }
                }
                
                room_remove_player(room, player);
            } else {
                LOG_DEBUG("server_handle_disconnect: unexpected disconnect path");
                /* Neocekavany disconnect - zachovej pro reconnect */
                if (opponent != NULL && opponent->socket_fd >= 0) {
                    LOG_DEBUG("server_handle_disconnect: sending PLAYER_STATUS to opponent");
                    protocol_create_player_status(response, sizeof(response),
                                                   player->nickname, STATUS_DISCONNECTED);
                    server_send_to_player(opponent, response);
                    LOG_DEBUG("server_handle_disconnect: PLAYER_STATUS sent");
                }
                
                /* Pozastav hru */
                LOG_DEBUG("server_handle_disconnect: game state = %d", room->game.state);
                if (room->game.state == GAME_STATE_PLAYING) {
                    LOG_DEBUG("server_handle_disconnect: pausing game");
                    game_pause(&room->game);
                    LOG_DEBUG("server_handle_disconnect: game paused");
                }
                
                /* Zachovej hrace pro reconnect */
                LOG_DEBUG("server_handle_disconnect: resetting player for reconnect");
                player_reset(player, true);
                LOG_DEBUG("server_handle_disconnect: updating max_fd");
                update_max_fd(server);
                LOG_DEBUG("server_handle_disconnect: done (reconnect path)");
                return;
            }
        } else {
            LOG_WARNING("server_handle_disconnect: room %d not found!", player->room_id);
        }
    }
    
    /* Uplne odpojeni */
    LOG_DEBUG("server_handle_disconnect: full disconnect, resetting player");
    player_reset(player, false);
    update_max_fd(server);
    LOG_DEBUG("server_handle_disconnect: done");
}

void server_handle_timeout(Server *server, Player *player) {
    if (player == NULL) return;
    
    LOG_WARNING("Player '%s' reconnect timeout expired",
                player->nickname[0] ? player->nickname : "(unknown)");
    
    char response[BUFFER_SIZE];
    
    /* Informuj protihrace a ukonci hru */
    if (player->room_id >= 0) {
        Room *room = room_find_by_id(server->rooms, server->config.max_rooms, player->room_id);
        if (room != NULL) {
            Player *opponent = room_get_opponent(room, player);
            
            if (opponent != NULL && opponent->socket_fd >= 0) {
                /* Protihrac vyhrává */
                protocol_create_game_over(response, sizeof(response),
                                           opponent->nickname, player->nickname);
                server_send_to_player(opponent, response);
                
                room_remove_player(room, opponent);
                player_set_state(opponent, PLAYER_STATE_LOBBY);
            }
            
            room_remove_player(room, player);
        }
    }
    
    player_reset(player, false);
}

void server_check_timeouts(Server *server) {
    time_t now = time(NULL);
    char buffer[BUFFER_SIZE];
    
    for (int i = 0; i < server->config.max_clients; i++) {
        Player *player = &server->players[i];
        
        if (!player->is_active) continue;
        
        /* Kontrola reconnect timeoutu */
        if (player->state == PLAYER_STATE_DISCONNECTED) {
            if (player_reconnect_timeout_expired(player)) {
                server_handle_timeout(server, player);
            }
            continue;
        }
        
        /* Kontrola LOGIN timeoutu - klient se pripojil, ale neposlal LOGIN */
        if (player->state == PLAYER_STATE_CONNECTING && player->socket_fd >= 0) {
            if ((now - player->last_activity) > LOGIN_TIMEOUT) {
                LOG_WARNING("Client at fd %d login timeout (no LOGIN received)", 
                            player->socket_fd);
                protocol_create_error(buffer, sizeof(buffer), ERR_NOT_LOGGED_IN,
                                      "Login timeout");
                server_send_to_player(player, buffer);
                server_handle_disconnect(server, player, false);
                continue;
            }
        }
        
        /* Kontrola, zda nepotrebuje PING */
        if (player->socket_fd >= 0 && player_needs_ping(player)) {
            snprintf(buffer, sizeof(buffer), "PING\n");
            if (server_send_to_player(player, buffer)) {
                player->last_ping = now;
                player->waiting_pong = true;
            }
        }
        
        /* Kontrola PONG timeoutu */
        if (player_pong_timeout_expired(player)) {
            LOG_WARNING("Player '%s' PONG timeout", 
                        player->nickname[0] ? player->nickname : "(unknown)");
            server_handle_disconnect(server, player, false);
        }
    }
}

bool server_parse_args(int argc, char *argv[], ServerConfig *config) {
    /* Vychozi hodnoty */
    strncpy(config->bind_address, DEFAULT_BIND_ADDR, sizeof(config->bind_address));
    config->port = DEFAULT_PORT;
    config->max_clients = DEFAULT_MAX_CLIENTS;
    config->max_rooms = DEFAULT_MAX_ROOMS;
    config->verbose = false;
    
    int opt;
    while ((opt = getopt(argc, argv, "a:p:c:r:vh")) != -1) {
        switch (opt) {
            case 'a':
                strncpy(config->bind_address, optarg, sizeof(config->bind_address) - 1);
                break;
            case 'p':
                config->port = atoi(optarg);
                if (config->port <= 0 || config->port > 65535) {
                    fprintf(stderr, "Invalid port: %s\n", optarg);
                    return false;
                }
                break;
            case 'c':
                config->max_clients = atoi(optarg);
                if (config->max_clients <= 0) {
                    fprintf(stderr, "Invalid max clients: %s\n", optarg);
                    return false;
                }
                break;
            case 'r':
                config->max_rooms = atoi(optarg);
                if (config->max_rooms <= 0) {
                    fprintf(stderr, "Invalid max rooms: %s\n", optarg);
                    return false;
                }
                break;
            case 'v':
                config->verbose = true;
                break;
            case 'h':
                return false; /* Vypise napovedu */
            default:
                return false;
        }
    }
    
    return true;
}

void server_print_usage(const char *program_name) {
    printf("Nim Game Server\n");
    printf("Usage: %s [options]\n\n", program_name);
    printf("Options:\n");
    printf("  -a ADDRESS   Bind address (default: %s)\n", DEFAULT_BIND_ADDR);
    printf("  -p PORT      Port number (default: %d)\n", DEFAULT_PORT);
    printf("  -c COUNT     Maximum clients (default: %d)\n", DEFAULT_MAX_CLIENTS);
    printf("  -r COUNT     Maximum rooms (default: %d)\n", DEFAULT_MAX_ROOMS);
    printf("  -v           Verbose mode (log to stdout instead of file)\n");
    printf("  -h           Show this help\n");
}

