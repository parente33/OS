/**
 * @file logger.h
 * @brief Logging utilities for the application
 */

#ifndef LOGGER_H
#define LOGGER_H

/**
 * @brief Logs a message to standard output
 *
 * @param format Format string (printf-style)
 * @param ... Variable arguments for the format string
 */
void util_log(const char *format, ...);

/**
 * @brief Logs an error message to standard error
 *
 * @param format Format string (printf-style)
 * @param ... Variable arguments for the format string
 */
void util_error(const char *format, ...);

#endif /* LOGGER_H */
