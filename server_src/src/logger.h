/**
 * @file logger.h
 * @brief Logovaci modul pro server
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>

/* ============================================
 * UROVNE LOGOVANI
 * ============================================ */

typedef enum {
    LOG_DEBUG,      /* Detailni ladici informace */
    LOG_INFO,       /* Bezne informace */
    LOG_WARNING,    /* Varovani */
    LOG_ERROR       /* Chyby */
} LogLevel;

/* ============================================
 * VEREJNE FUNKCE
 * ============================================ */

/**
 * Inicializuje logger
 * @param filename Cesta k logovaci souboru (NULL = stdout)
 * @param min_level Minimalni uroven logovani
 * @return true pri uspechu
 */
bool logger_init(const char *filename, LogLevel min_level);

/**
 * Ukonci logger a zavre soubor
 */
void logger_close(void);

/**
 * Nastavi minimalni uroven logovani
 */
void logger_set_level(LogLevel level);

/**
 * Zapise log zpravu
 * @param level Uroven zpravy
 * @param format Format string (jako printf)
 * @param ... Argumenty
 */
void logger_log(LogLevel level, const char *format, ...);

/* ============================================
 * MAKRA PRO SNADNEJSI POUZITI
 * ============================================ */

#define LOG_DEBUG(...)   logger_log(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)    logger_log(LOG_INFO, __VA_ARGS__)
#define LOG_WARNING(...) logger_log(LOG_WARNING, __VA_ARGS__)
#define LOG_ERROR(...)   logger_log(LOG_ERROR, __VA_ARGS__)

#endif /* LOGGER_H */

