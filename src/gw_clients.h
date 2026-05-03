#pragma once

#include <stddef.h>

/**
 * @brief Writes an HTML snippet listing TCP clients currently connected
 *        to the MySensors gateway port (5003) into the provided buffer.
 *        Includes remote IP, port, TCP state, and connection duration.
 */
void buildGwClientsHtml(char *buf, size_t buflen);
