/**
 * @file logger.c
 * @brief Implementace logovaciho modulu
 */

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

/* ============================================
 * PRIVATNI PROMENNE
 * ============================================ */

static FILE *log_file = NULL;
static LogLevel min_log_level = LOG_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ============================================
 * POMOCNE FUNKCE
 * ============================================ */

/**
 * Vrati textovou reprezentaci urovne logovani
 */
static const char* level_to_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG:   return "DEBUG";
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR:   return "ERROR";
        default:          return "UNKNOWN";
    }
}

/**
 * Vrati aktualni cas jako formatovany retezec
 */
static void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* ============================================
 * IMPLEMENTACE VEREJNYCH FUNKCI
 * ============================================ */

bool logger_init(const char *filename, LogLevel min_level) {
    pthread_mutex_lock(&log_mutex);
    
    /* Zavreni predchoziho souboru */
    if (log_file != NULL && log_file != stdout) {
        fclose(log_file);
    }
    
    min_log_level = min_level;
    
    /* Otevreni noveho souboru nebo stdout */
    if (filename != NULL) {
        log_file = fopen(filename, "a");
        if (log_file == NULL) {
            log_file = stdout;
            pthread_mutex_unlock(&log_mutex);
            fprintf(stderr, "Warning: Cannot open log file '%s', using stdout\n", filename);
            return false;
        }
    } else {
        log_file = stdout;
    }
    
    pthread_mutex_unlock(&log_mutex);
    
    /* Uvodni zprava */
    logger_log(LOG_INFO, "Logger initialized (level: %s)", level_to_string(min_level));
    
    return true;
}

void logger_close(void) {
    pthread_mutex_lock(&log_mutex);
    
    if (log_file != NULL && log_file != stdout) {
        fclose(log_file);
    }
    log_file = NULL;
    
    pthread_mutex_unlock(&log_mutex);
}

void logger_set_level(LogLevel level) {
    pthread_mutex_lock(&log_mutex);
    min_log_level = level;
    pthread_mutex_unlock(&log_mutex);
}

void logger_log(LogLevel level, const char *format, ...) {
    if (level < min_log_level) {
        return;
    }
    
    pthread_mutex_lock(&log_mutex);
    
    FILE *output = (log_file != NULL) ? log_file : stdout;
    
    /* Cas a uroven */
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    fprintf(output, "[%s] [%s] ", timestamp, level_to_string(level));
    
    /* Samotna zprava */
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);
    
    fprintf(output, "\n");
    fflush(output);
    
    pthread_mutex_unlock(&log_mutex);
}

