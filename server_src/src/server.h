/**
 * @file server.h
 * @brief Hlavni serverovy modul - socket handling, select() loop
 */

#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <netinet/in.h>
#include <sys/select.h>
#include "player.h"
#include "room.h"
#include "../include/config.h"

/* ============================================
 * KONFIGURACE SERVERU
 * ============================================ */

typedef struct {
    char bind_address[64];
    int port;
    int max_clients;
    int max_rooms;
    bool verbose;           /* Verbose mode - log to stdout */
} ServerConfig;

/* ============================================
 * STAV SERVERU
 * ============================================ */

typedef struct {
    int listen_fd;                  /* Socket pro naslouchani */
    ServerConfig config;            /* Konfigurace */
    Player *players;                /* Pole hracu */
    Room *rooms;                    /* Pole mistnosti */
    bool running;                   /* Server bezi? */
    fd_set read_fds;               /* File descriptory pro select */
    int max_fd;                     /* Nejvyssi fd pro select */
} Server;

/* ============================================
 * VEREJNE FUNKCE
 * ============================================ */

/**
 * Inicializuje server s danou konfiguraci
 * @param server Ukazatel na strukturu serveru
 * @param config Konfigurace serveru
 * @return true pri uspechu
 */
bool server_init(Server *server, const ServerConfig *config);

/**
 * Spusti hlavni smycku serveru
 * @param server Ukazatel na server
 */
void server_run(Server *server);

/**
 * Ukonci server
 * @param server Ukazatel na server
 */
void server_shutdown(Server *server);

/**
 * Odesle zpravu klientovi
 * @param player Cilovy hrac
 * @param message Zprava k odeslani
 * @return true pri uspechu
 */
bool server_send_to_player(Player *player, const char *message);

/**
 * Odesle zpravu vsem hracum v mistnosti
 * @param room Mistnost
 * @param message Zprava
 * @param except Hrac, kteremu se zprava neposila (muze byt NULL)
 */
void server_broadcast_to_room(Room *room, const char *message, Player *except);

/**
 * Odesle zpravu vsem hracum v lobby
 * @param server Server
 * @param message Zprava
 */
void server_broadcast_to_lobby(Server *server, const char *message);

/**
 * Zpracuje prijatou zpravu od klienta
 * @param server Server
 * @param player Odesilajici hrac
 * @param message Prijata zprava
 */
void server_handle_message(Server *server, Player *player, const char *message);

/**
 * Zpracuje odpojeni klienta
 * @param server Server
 * @param player Odpojeny hrac
 * @param graceful Bylo odpojeni s LOGOUT zpravou?
 */
void server_handle_disconnect(Server *server, Player *player, bool graceful);

/**
 * Zpracuje timeout hrace (pro reconnect)
 * @param server Server
 * @param player Hrac
 */
void server_handle_timeout(Server *server, Player *player);

/**
 * Zkontroluje a zpracuje timeouty
 * @param server Server
 */
void server_check_timeouts(Server *server);

/**
 * Parsuje argumenty prikazove radky
 * @param argc Pocet argumentu
 * @param argv Argumenty
 * @param config Vystupni konfigurace
 * @return true pri uspechu
 */
bool server_parse_args(int argc, char *argv[], ServerConfig *config);

/**
 * Vypise napovedu
 * @param program_name Nazev programu
 */
void server_print_usage(const char *program_name);

#endif /* SERVER_H */

