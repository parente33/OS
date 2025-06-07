/**
 * @file status.h
 * @brief Common status codes and error handling
 *
 * This module defines the standard status codes used throughout the
 * application for error handling and operation results.
 */

#ifndef STATUS_H
#define STATUS_H

/* === Status Codes ============================================== */

/**
 * @brief Operation completed successfully
 */
#define OS_OK        0

/**
 * @brief General error condition
 *
 * Used when an operation fails for any reason not covered by
 * more specific error codes.
 */
#define OS_ERROR    -1

/**
 * @brief Operation should be retried
 *
 * Used when an operation cannot be completed immediately but
 * should be retried later (e.g., resource temporarily unavailable).
 */
#define OS_AGAIN    -2

/**
 * @brief System is shutting down
 *
 * Used to indicate that the system is in the process of
 * shutting down and operations should be terminated gracefully.
 */
#define OS_SHUTDOWN -3

#endif /* STATUS_H */