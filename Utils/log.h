#ifndef UTILS_LOG_H
#define UTILS_LOG_H

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file log.h 
 * @brief Provides a custom logging system for any file to write to. 
 * List of macros: 
 * LogRingBufMaxSize -> Max size(in bytes) that the logger can hold. 
 * LogMaxMessageSize -> Max size(in bytes) that any one message in the logger can hold. 
 * LogDrainChunkSize -> Max size(in bytes) the logger will drain when logDrain() is called. 
 * 
 * The following macros can be defined to enable/disable certain types of logging: 
 * LOG_LEVEL_NONE -> Completely disables logging. 
 * LOG_LEVEL_ERROR -> Enables logging for errors only. 
 * LOG_LEVEL_WARN -> Enables logging for warnings and errors only. 
 * LOG_LEVEL_INFO -> Enables all logging. 
 * 
 * Log_LEVEL_INFO is defined by default. 
**/

#define LogRingBufMaxSize   512
#define LogMaxMessageSize   128
#define LogDrainChunkSize   64

#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3

//Override from the build.
#ifndef LOG_LEVEL
    #define LOG_LEVEL LOG_LEVEL_INFO
#endif

/**
 * @brief Wrapper to call logWriteV(...). 
 *
 *
 * @param[in] level uint8_t for the level(as defined in log.c) that the message has. 
 * @param[in] tag   const char * for the message associated with the level. 
 * @param[in] fmt   const char * format string that specifies how the extra arguments should be written. 
 **/
void logWrite(uint8_t level, const char *tag, const char *fmt, ...);

/**
 * @brief Function called by logWrite(...) that writes to the log buffer. 
 *
 *
 * @param[in] level uint8_t for the level(as defined in log.c) that the message has. 
 * @param[in] tag   const char * for the message associated with the level. 
 * @param[in] fmt   const char * format string that specifies how the extra arguments should be written. 
 * @param[in] args  va_list variable containing the extra arguments to write to the log buffer. 
 **/
void logWriteV(uint8_t level, const char *tag, const char *fmt, va_list args);

/**
 * @brief Drains the log by a max of LogDrainChunkSize by printing the log over UART. 
 *
**/ 
void logDrain(void);

#if LOG_LEVEL >= LOG_LEVEL_ERROR
    #define LOG_ERROR(tag, fmt, ...) logWrite(LOG_LEVEL_ERROR, (tag), (fmt), ##__VA_ARGS__)
#else
    #define LOG_ERROR(tag, fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
    #define LOG_WARN(tag, fmt, ...) logWrite(LOG_LEVEL_WARN, (tag), (fmt), ##__VA_ARGS__)
#else
    #define LOG_WARN(tag, fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
    #define LOG_INFO(tag, fmt, ...) logWrite(LOG_LEVEL_INFO, (tag), (fmt), ##__VA_ARGS__)
#else
    #define LOG_INFO(tag, fmt, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* UTILS_LOG_H */
