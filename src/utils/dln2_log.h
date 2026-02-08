#include <stdio.h>

// Define log levels
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3

// Set current log level (change this to control what gets printed)
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

// Log macros
#define LOG_ERROR(fmt, ...)                                                    \
  do {                                                                         \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_ERROR)                                  \
      printf("[ERROR] " fmt "\n", ##__VA_ARGS__);                              \
  } while (0)
#define LOG_WARN(fmt, ...)                                                     \
  do {                                                                         \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_WARNING)                                \
      printf("[WARN]  " fmt "\n", ##__VA_ARGS__);                              \
  } while (0)
#define LOG_INFO(fmt, ...)                                                     \
  do {                                                                         \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_INFO)                                   \
      printf("[INFO]  " fmt "\n", ##__VA_ARGS__);                              \
  } while (0)
#define LOG_DEBUG(fmt, ...)                                                    \
  do {                                                                         \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG)                                  \
      printf("[DEBUG] " fmt "\n", ##__VA_ARGS__);                              \
  } while (0)
