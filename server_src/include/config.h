/**
 * @file config.h
 * @brief Konfiguracni konstanty pro Nim server
 * 
 * Tento soubor obsahuje vsechny konfiguracni konstanty,
 * limity a vychozi hodnoty pro server.
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ============================================
 * SITOVE KONSTANTY
 * ============================================ */

/** Vychozi port pro naslouchani */
#define DEFAULT_PORT 10000

/** Vychozi IP adresa pro bind */
#define DEFAULT_BIND_ADDR "0.0.0.0"

/** Maximalni velikost bufferu pro prijem zprav */
#define BUFFER_SIZE 1024

/** Maximalni delka jedne zpravy */
#define MAX_MESSAGE_LENGTH 512

/** Timeout pro select() v sekundach */
#define SELECT_TIMEOUT_SEC 1

/** Timeout pro select() v mikrosekundach */
#define SELECT_TIMEOUT_USEC 0

/* ============================================
 * LIMITY SERVERU
 * ============================================ */

/** Vychozi maximalni pocet mistnosti */
#define DEFAULT_MAX_ROOMS 10

/** Vychozi maximalni pocet klientu */
#define DEFAULT_MAX_CLIENTS 50

/** Pocet hracu na mistnost */
#define PLAYERS_PER_ROOM 2

/** Maximalni delka prezdivky */
#define MAX_NICKNAME_LENGTH 32

/** Maximalni delka nazvu mistnosti */
#define MAX_ROOM_NAME_LENGTH 64

/* ============================================
 * HERNI KONSTANTY (NIM)
 * ============================================ */

/** Pocatecni pocet kaminku */
#define INITIAL_STONES 21

/** Minimalni pocet kaminku k odebrani */
#define MIN_TAKE 1

/** Maximalni pocet kaminku k odebrani */
#define MAX_TAKE 3

/** Pocet preskoceni tahu na hrace */
#define SKIPS_PER_PLAYER 1

/* ============================================
 * RECONNECTION A TIMEOUTY
 * ============================================ */

/** Kratkodoby timeout pro reconnect (sekundy) */
#define SHORT_DISCONNECT_TIMEOUT 30

/** Interval pro ping zpravy (sekundy) */
#define PING_INTERVAL 10

/** Timeout pro odpoved na ping (sekundy) */
#define PING_TIMEOUT 5

/* ============================================
 * VALIDACE A BEZPECNOST
 * ============================================ */

/** Maximalni pocet nevalidnich zprav pred odpojenim */
#define MAX_INVALID_MESSAGES 3

/** Maximalni pocet pokusu o reconnect */
#define MAX_RECONNECT_ATTEMPTS 6

/** Maximalni delka zpravy bez ukoncovatka (ochrana proti flood) */
#define MAX_MESSAGE_WITHOUT_NEWLINE 256

/** Minimalni interval mezi zpravami v ms (ochrana proti flood) */
#define MIN_MESSAGE_INTERVAL_MS 50

/** Maximalni pocet zprav za sekundu */
#define MAX_MESSAGES_PER_SECOND 20

/** Timeout pro LOGIN po pripojeni (sekundy) */
#define LOGIN_TIMEOUT 30

/* ============================================
 * LOGGING
 * ============================================ */

/** Cesta k logovacemu souboru */
#define LOG_FILE "nim_server.log"

/** Maximalni delka log zpravy */
#define MAX_LOG_MESSAGE_LENGTH 256

/* ============================================
 * PROTOKOL - ODDELOVACE
 * ============================================ */

/** Oddelovac parametru ve zprave */
#define MSG_DELIMITER ';'

/** Ukoncovac zpravy */
#define MSG_TERMINATOR '\n'

/** Oddelovac polozek v seznamu */
#define LIST_DELIMITER ','

#endif /* CONFIG_H */

