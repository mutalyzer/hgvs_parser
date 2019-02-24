#if !defined(__hgvs_interface_h__)
#define __hgvs_interface_h__


#include <stddef.h>
#include <stdio.h>

#if defined(ANSI)
#include <unistd.h>
#endif


#define ANSI_RESET       "\033[0m"
#define ANSI_BLACK       "\033[30m"
#define ANSI_RED         "\033[31m"
#define ANSI_GREEN       "\033[32m"
#define ANSI_YELLOW      "\033[33m"
#define ANSI_BLUE        "\033[34m"
#define ANSI_MAGENTA     "\033[35m"
#define ANSI_CYAN        "\033[36m"
#define ANSI_WHITE       "\033[37m"
#define ANSI_BOLDBLACK   "\033[1m\033[30m"
#define ANSI_BOLDRED     "\033[1m\033[31m"
#define ANSI_BOLDGREEN   "\033[1m\033[32m"
#define ANSI_BOLDYELLOW  "\033[1m\033[33m"
#define ANSI_BOLDBLUE    "\033[1m\033[34m"
#define ANSI_BOLDMAGENTA "\033[1m\033[35m"
#define ANSI_BOLDCYAN    "\033[1m\033[36m"
#define ANSI_BOLDWHITE   "\033[1m\033[37m"


enum HGVS_Format
{
    HGVS_Format_console,
} HGVS_Format;


static inline int
is_tty(FILE const* const stream)
{
#if defined(ANSI)
    return (stream == stdout && isatty(STDOUT_FILENO)) ||
           (stream == stderr && isatty(STDERR_FILENO));
#else
    (void) stream;
    return 0;
#endif
} // is_tty


static inline size_t
fprintf_operator(FILE*                  stream,
                 enum HGVS_Format const fmt,
                 char const             op)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%c%s", is_tty(stream) ? ANSI_MAGENTA : "",
                                             op,
                                             is_tty(stream) ? ANSI_RESET : "");
    } // switch
} // switch


static inline size_t
fprintf_variant(FILE*                  stream,
                enum HGVS_Format const fmt,
                char const* const      tok)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%s%s", is_tty(stream) ? ANSI_GREEN : "",
                                             tok,
                                             is_tty(stream) ? ANSI_RESET : "");
    } // switch
} // switch


static inline size_t
fprintf_number(FILE*                  stream,
               enum HGVS_Format const fmt,
               size_t const           num)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%zu%s", is_tty(stream) ? ANSI_CYAN : "",
                                              num,
                                              is_tty(stream) ? ANSI_RESET : "");
    } // switch
} // switch


static inline size_t
fprintf_unknown(FILE*                  stream,
                enum HGVS_Format const fmt,
                char const             op)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%c%s", is_tty(stream) ? ANSI_CYAN : "",
                                             op,
                                             is_tty(stream) ? ANSI_RESET : "");
    } // switch
} // switch


static inline size_t
fprintf_string(FILE*                  stream,
               enum HGVS_Format const fmt,
               char const* const      ptr,
               size_t const           len)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%.*s%s", is_tty(stream) ? ANSI_BOLDWHITE : "",
                                               (int) len,
                                               ptr,
                                               is_tty(stream) ? ANSI_RESET : "");
    } // switch
} // switch


static inline size_t
fprintf_char(FILE*                  stream,
             enum HGVS_Format const fmt,
             char const             ch)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%c%s", is_tty(stream) ? ANSI_BOLDWHITE : "",
                                             ch,
                                             is_tty(stream) ? ANSI_RESET : "");
    } // switch
} // switch


static inline size_t
fprintf_error(FILE*                  stream,
              enum HGVS_Format const fmt,
              int const              indent,
              char const* const      msg)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%*s%s^ %serror: %s%s%s\n", indent,
                                                               "",
                                                               is_tty(stream) ? ANSI_BOLDGREEN : "",
                                                               is_tty(stream) ? ANSI_BOLDMAGENTA : "",
                                                               is_tty(stream) ? ANSI_BOLDWHITE : "",
                                                               msg,
                                                               is_tty(stream) ? ANSI_RESET : "");
    } // switch
} // fprintf_error


static inline size_t
fprintf_failed(FILE* stream)
{
    return fprintf(stream, "%sfailed.%s\n", is_tty(stream) ? ANSI_BOLDRED : "",
                                            is_tty(stream) ? ANSI_RESET : "");
} // fprintf_failed


static inline size_t
fprintf_accept(FILE* stream)
{
    return fprintf(stream, "%saccepted.%s\n", is_tty(stream) ? ANSI_BOLDGREEN : "",
                                              is_tty(stream) ? ANSI_RESET : "");
} // fprintf_accept


#endif
