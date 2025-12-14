/**
 * @file main.c
 * @brief Vstupni bod serveru pro hru Nim
 * 
 * Server pro sitovou hru Nim s architekturou klient-server.
 * Podporuje vice soucasnych her v oddelených mistnostech.
 * 
 * @author Student
 * @date 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "logger.h"
#include "../include/config.h"

/**
 * Hlavni funkce
 */
int main(int argc, char *argv[]) {
    ServerConfig config;
    Server server;
    
    /* Parsovani argumentu */
    if (!server_parse_args(argc, argv, &config)) {
        server_print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    /* Inicializace loggeru */
    /* Verbose mode (-v) loguje na stdout, jinak do souboru */
    const char *log_file = config.verbose ? NULL : LOG_FILE;
    if (!logger_init(log_file, LOG_INFO)) {
        fprintf(stderr, "Warning: Failed to initialize logger to file, using stdout\n");
    }
    
    LOG_INFO("===========================================");
    LOG_INFO("Nim Game Server Starting");
    LOG_INFO("===========================================");
    LOG_INFO("Configuration:");
    LOG_INFO("  Bind address: %s", config.bind_address);
    LOG_INFO("  Port: %d", config.port);
    LOG_INFO("  Max clients: %d", config.max_clients);
    LOG_INFO("  Max rooms: %d", config.max_rooms);
    LOG_INFO("Game settings:");
    LOG_INFO("  Initial stones: %d", INITIAL_STONES);
    LOG_INFO("  Min take: %d", MIN_TAKE);
    LOG_INFO("  Max take: %d", MAX_TAKE);
    LOG_INFO("  Skips per player: %d", SKIPS_PER_PLAYER);
    
    /* Inicializace serveru */
    if (!server_init(&server, &config)) {
        LOG_ERROR("Failed to initialize server");
        logger_close();
        return EXIT_FAILURE;
    }
    
    /* Hlavni smycka */
    server_run(&server);
    
    /* Uklid */
    logger_close();
    
    return EXIT_SUCCESS;
}

