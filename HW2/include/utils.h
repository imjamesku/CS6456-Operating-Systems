#ifndef FLORAL_UTILS_H
#define FLORAL_UTILS_H

/*
 * The macros below are for debugging and printing out verbose output. They are
 * similar to the Linux debug macros in that defining DEBUG will enable debug
 * output. The verbose macro will print if DEBUG was defined or if the `active`
 * parameter is true.
 */
#ifdef DEBUG
    #define DEBUG_ 1
    #define D(x) x
#else
    #define DEBUG_ 0
    #define D(x)
#endif

#define debug_cond(condition, stream, ...) \
    do { \
        if ((condition)) { \
            fprintf(stream, "%s:%d:%s(): ", __FILE__, __LINE__, __func__); \
            fprintf(stream, __VA_ARGS__); \
        } \
    } while (0)

#define debug(...) debug_cond(DEBUG_, stderr, __VA_ARGS__)

#define verbose(active, ...) debug_cond(DEBUG_ || active, stderr, __VA_ARGS__);

#define if_debug \
  if (DEBUG_)

/*
 * The macros below are helper macros for printing error messages.
 * Additionally, err_exit will exit with a failing return code after printing
 * the message.
 */
#define print_err(...) \
    do { \
        fprintf(stderr, "ERROR: "); \
        fprintf(stderr, __VA_ARGS__); \
        fflush(stderr); \
    } while (0)

#define err_exit(...) \
    do { \
        print_err(__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } while (0)

#define assert_malloc(ptr) \
    do { \
        if (!(ptr)) { \
            err_exit("Couldn't allocate memory for '%s' at %s:%d[%s()].\n", \
                #ptr, __FILE__, __LINE__, __func__); \
        }; \
    } while (0)

#define assert(cond) \
    do { \
        if (!(cond)) { \
            err_exit("Assert '%s' failed %s:%d[%s()].\n", \
                #cond, __FILE__, __LINE__, __func__); \
        }; \
    } while (0)

/*
 * A simple macro that allows a variable to pass through unused checks even if
 * it is actually unused.
 */
#define UNUSED(var) \
  (void)(var)

#endif
