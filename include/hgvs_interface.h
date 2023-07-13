#ifndef HGVS_INTERFACE_H
#define HGVS_INTERFACE_H


#include <stddef.h>
#include <stdio.h>

#if defined(ANSI)
#include <unistd.h>
#endif


static char const* const HGVS_ANSI_RESET       = "\033[0m";
static char const* const HGVS_ANSI_BLACK       = "\033[30m";
static char const* const HGVS_ANSI_RED         = "\033[31m";
static char const* const HGVS_ANSI_GREEN       = "\033[32m";
static char const* const HGVS_ANSI_YELLOW      = "\033[33m";
static char const* const HGVS_ANSI_BLUE        = "\033[34m";
static char const* const HGVS_ANSI_MAGENTA     = "\033[35m";
static char const* const HGVS_ANSI_CYAN        = "\033[36m";
static char const* const HGVS_ANSI_WHITE       = "\033[37m";
static char const* const HGVS_ANSI_BOLDBLACK   = "\033[1m\033[30m";
static char const* const HGVS_ANSI_BOLDRED     = "\033[1m\033[31m";
static char const* const HGVS_ANSI_BOLDGREEN   = "\033[1m\033[32m";
static char const* const HGVS_ANSI_BOLDYELLOW  = "\033[1m\033[33m";
static char const* const HGVS_ANSI_BOLDBLUE    = "\033[1m\033[34m";
static char const* const HGVS_ANSI_BOLDMAGENTA = "\033[1m\033[35m";
static char const* const HGVS_ANSI_BOLDCYAN    = "\033[1m\033[36m";
static char const* const HGVS_ANSI_BOLDWHITE   = "\033[1m\033[37m";


enum HGVS_Format
{
    HGVS_Format_plain,
    HGVS_Format_console,
};


static inline int
HGVS_is_tty(FILE const* const stream)
{
#if defined(ANSI)
    return (stream == stdout && isatty(STDOUT_FILENO)) ||
           (stream == stderr && isatty(STDERR_FILENO));
#else
    (void) stream;
    return 0;
#endif
} // HGVS_is_tty


static inline size_t
HGVS_fprintf_operator(FILE*                  stream,
                      enum HGVS_Format const fmt,
                      char const             op)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%c%s",
                           HGVS_is_tty(stream) ? HGVS_ANSI_MAGENTA : "",
                           op,
                           HGVS_is_tty(stream) ? HGVS_ANSI_RESET : "");
        default:
            break;
    } // switch
    return fprintf(stream, "%c", op);
} // HGVS_fprintf_operator


static inline size_t
HGVS_fprintf_keyword(FILE*                  stream,
                     enum HGVS_Format const fmt,
                     char const* const      tok)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%s%s",
                           HGVS_is_tty(stream) ? HGVS_ANSI_GREEN : "",
                           tok,
                           HGVS_is_tty(stream) ? HGVS_ANSI_RESET : "");
        default:
            break;
    } // switch
    return fprintf(stream, "%s", tok);
} // HGVS_fprintf_keyword


static inline size_t
HGVS_fprintf_number(FILE*                  stream,
                    enum HGVS_Format const fmt,
                    size_t const           num)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%zu%s",
                           HGVS_is_tty(stream) ? HGVS_ANSI_CYAN : "",
                           num,
                           HGVS_is_tty(stream) ? HGVS_ANSI_RESET : "");
        default:
            break;
    } // switch
    return fprintf(stream, "%zu", num);
} // HGVS_fprintf_number


static inline size_t
HGVS_fprintf_string(FILE*                  stream,
                    enum HGVS_Format const fmt,
                    char const* const      ptr,
                    int const              len)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%.*s%s",
                           HGVS_is_tty(stream) ? HGVS_ANSI_BOLDWHITE : "",
                           len,
                           ptr,
                           HGVS_is_tty(stream) ? HGVS_ANSI_RESET : "");
        default:
            break;
    } // switch
    return fprintf(stream, "%.*s", len, ptr);
} // HGVS_fprintf_string


static inline size_t
HGVS_fprintf_char(FILE*                  stream,
                  enum HGVS_Format const fmt,
                  char const             ch)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%s%c%s",
                           HGVS_is_tty(stream) ? HGVS_ANSI_BOLDWHITE : "",
                           ch,
                           HGVS_is_tty(stream) ? HGVS_ANSI_RESET : "");
        default:
            break;
    } // switch
    return fprintf(stream, "%c", ch);
} // HGVS_fprintf_char


static inline size_t
HGVS_fprintf_error(FILE*                  stream,
                   enum HGVS_Format const fmt,
                   int const              indent,
                   char const* const      msg)
{
    switch (fmt)
    {
        case HGVS_Format_console:
            return fprintf(stream, "%*s%s^ %serror: %s%s%s\n",
                           indent,
                           "",
                           HGVS_is_tty(stream) ? HGVS_ANSI_BOLDGREEN : "",
                           HGVS_is_tty(stream) ? HGVS_ANSI_BOLDMAGENTA : "",
                           HGVS_is_tty(stream) ? HGVS_ANSI_BOLDWHITE : "",
                           msg,
                           HGVS_is_tty(stream) ? HGVS_ANSI_RESET : "");
        default:
            break;
    } // switch
    return fprintf(stream, "%*s", indent, msg);
} // HGVS_fprintf_error


static inline size_t
HGVS_fprintf_failed(FILE* stream)
{
    return fprintf(stream, "%sfailed.%s\n",
                   HGVS_is_tty(stream) ? HGVS_ANSI_BOLDRED : "",
                   HGVS_is_tty(stream) ? HGVS_ANSI_RESET : "");
} // HGVS_fprintf_failed


static inline size_t
HGVS_fprintf_accept(FILE* stream)
{
    return fprintf(stream, "%saccepted.%s\n",
                   HGVS_is_tty(stream) ? HGVS_ANSI_BOLDGREEN : "",
                   HGVS_is_tty(stream) ? HGVS_ANSI_RESET : "");
} // HGVS_fprintf_accept


#endif
