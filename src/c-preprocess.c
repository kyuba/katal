/*
 * This file is part of the kyuba.org Katal project.
 * See the appropriate repository at http://git.kyuba.org/ for exact file
 * modification records.
*/

/*
 * Copyright (c) 2010, Kyuba Project Members
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#include <curie/memory.h>
#include <curie/multiplex.h>
#include <curie/filesystem.h>
#include <sievert/immutable.h>
#include <katal/c.h>

define_string (str_slash, "/");

#define KATAL_CPP_INCLUDING                (1 << 0x1f)
#define KATAL_CPP_IN_STRING                (1 << 0x1e)
#define KATAL_CPP_POST_STRING              (1 << 0x1d)
#define KATAL_CPP_POST_NEWLINE             (1 << 0x1c)
#define KATAL_CPP_IN_ESCAPE                (1 << 0x1b)
#define KATAL_CPP_IN_COMMENT_UNTIL_NEWLINE (1 << 0x1a)
#define KATAL_CPP_IN_COMMENT               (1 << 0x19)
#define KATAL_CPP_POST_COMMENT             (1 << 0x18)
#define KATAL_CPP_IN_INSTRUCTION           (1 << 0x17)
#define KATAL_CPP_PROBABLY_DEFINE          (1 << 0x16)
#define KATAL_CPP_PROBABLY_INCLUDE         (1 << 0x15)
#define KATAL_CPP_PROBABLY_IF              (1 << 0x14)
#define KATAL_CPP_PROBABLY_ELSE            (1 << 0x13)
#define KATAL_CPP_PROBABLY_ELIF            (1 << 0x12)
#define KATAL_CPP_PROBABLY_ENDIF           (1 << 0x11)
#define KATAL_CPP_IN_DEFINE                (1 << 0x10)
#define KATAL_CPP_IN_INCLUDE               (1 << 0x0f)
#define KATAL_CPP_IN_IF                    (1 << 0x0e)
#define KATAL_CPP_IN_IFDEF                 (1 << 0x0d)
#define KATAL_CPP_IN_ELSE                  (1 << 0x0c)
#define KATAL_CPP_IN_ELIF                  (1 << 0x0b)
#define KATAL_CPP_IN_ENDIF                 (1 << 0x0a)
#define KATAL_CPP_CONDITIONAL_SKIPPING     (1 << 0x09)
#define KATAL_CPP_MAY_CLOSE                (1 << 0x08)

#define KATAL_CPP_INCLUDE_IN_STRING        (1 << 0x1f)
#define KATAL_CPP_INCLUDE_SEARCH_IN_BASE   (1 << 0x1e)

struct macro_data
{
    const char **parameters;
    const char **replacement;
};

struct ppdata
{
    unsigned int options;
    struct io *in;
    struct io *out;
    const char **include;
    const char *base;
    const char **defines;
    struct macro_data **defines_data;
    unsigned int tmp;
    const char *tstring;
    unsigned int depth;
    void (*on_end_of_input)(void *);
    void (*on_notice)(enum katal_notice, const char *, void *);
    void *aux;
};

static void on_cpp_read (struct io *in, void *aux);

static void on_recursion_end_of_input (void *aux)
{
    struct ppdata *d = (struct ppdata *)aux;

    d->options ^= KATAL_CPP_INCLUDING;

    on_cpp_read (d->in, d);
}

static void on_recursion_notice (enum katal_notice t, const char *s, void *aux)
{
    struct ppdata *d = (struct ppdata *)aux;

    d->on_notice (t, s, d->aux);
}

static void on_cpp_read (struct io *in, void *aux)
{
    struct ppdata *d = (struct ppdata *)aux;

    if (!(d->options & KATAL_CPP_INCLUDING))
    {
        /* don't bother doing anything if currently a different file is being
         * included into the output file. */

        unsigned long  i     = in->position;
        unsigned int   opt   = d->options;
        unsigned int   tmp   = d->tmp;
        char          *b     = in->buffer;
        unsigned int   depth = d->depth;

        for (; i < in->length; i++)
        {
            if (opt & KATAL_CPP_IN_INSTRUCTION)
            {
                /* handle cpp instructions here... first we gotta figure out
                 * if we support the particular instruction, of course: */

                if (!(opt & (KATAL_CPP_IN_INCLUDE | KATAL_CPP_IN_DEFINE |
                             KATAL_CPP_IN_IF      | KATAL_CPP_IN_IFDEF  |
                             KATAL_CPP_IN_ELIF    | KATAL_CPP_IN_ELSE   |
                             KATAL_CPP_IN_ENDIF)) &&
                     (tmp == 0))
                {
                    switch (b[i])
                    {
                        case ' ':
                        case '\v':
                        case '\t':
                            /* ignore initial whitespace after the #; it's ugly
                             * but i've seen people use stuff like #<indent>if
                             * and the like. */
                            break;
                        case 'd':
                            /* starts with a 'd', so it's probably a #define */
                            opt |= KATAL_CPP_PROBABLY_DEFINE;
                            tmp++;
                            break;
                        case 'i':
                            /* starts with an 'i', so it's probably an
                             * #include; might also turn into an #if, etc */
                            opt |= KATAL_CPP_PROBABLY_INCLUDE;
                            tmp++;
                            break;
                        case 'e':
                            /* #else, #elif or #endif */
                            opt |= KATAL_CPP_PROBABLY_ENDIF;
                            tmp++;
                            break;
                        default:
                            /* unknown instruction; just dump it verbatim */
                            io_collect (d->out, "#", 1);
                            opt ^= KATAL_CPP_IN_INSTRUCTION;
                    }
                }
                else if (opt & KATAL_CPP_IN_INCLUDE)
                {
                    /* handle #include */
                    if (tmp & KATAL_CPP_INCLUDE_IN_STRING)
                    {
                        switch (b[i])
                        {
                            case '>':
                            case '"':
                                tmp ^= KATAL_CPP_INCLUDE_IN_STRING;
                                break;
                            default:
                                tmp++;
                        }

                        if (!(tmp & KATAL_CPP_INCLUDE_IN_STRING))
                        {
                            sexpr fname, path;
                            unsigned int y = 0;
                            const char **list = d->include;

                            /* done reading in a filename now */
                            b[i] = 0;

                            fname = make_string
                                (b + i -
                                    (tmp & ~KATAL_CPP_INCLUDE_SEARCH_IN_BASE));


                            if ((tmp & KATAL_CPP_INCLUDE_SEARCH_IN_BASE) &&
                                (d->base != (const char *)0))
                            {
                                path = sx_join
                                    (make_string (d->base), str_slash, fname);

                                if (truep (filep (path)))
                                {
                                    goto do_include;
                                }
                            }

                          retry_include_scan:
                            while ((list != (const char **)0) &&
                                   (list[y] != (const char *)0))
                            {
                                path = sx_join
                                    (make_string (list[y]), str_slash, fname);

                                if (truep (filep (path)))
                                {
                                  do_include:
                                    in->position = i;
                                    d->options   = (opt | KATAL_CPP_INCLUDING)
                                                 ^ (KATAL_CPP_IN_INCLUDE |
                                                    KATAL_CPP_IN_INSTRUCTION);
                                    d->tmp       = 0;
                                    d->depth     = depth;

                                    io_commit (d->out);
                                    
                                    katal_c_preprocess_file
                                        (0, sx_string (path), d->out,
                                         d->include, d->defines,
                                         on_recursion_end_of_input,
                                         on_recursion_notice,
                                         (void *)d);
                                    opt |= KATAL_CPP_INCLUDING;

                                    /* returning here since we now need to
                                     * include the other file before
                                     * continuing to process this one. */
                                    return;
                                }

                                y++;
                            }

                            if (!(opt & KATAL_CPP_INCLUDING) &&
                                (list != katal_include_directories))
                            {
                                list = katal_include_directories;
                                goto retry_include_scan;
                            }

                            /* done processing */
                            opt ^= (KATAL_CPP_IN_INCLUDE |
                                    KATAL_CPP_IN_INSTRUCTION);
                        }
                    }
                    else
                    {
                        switch (b[i])
                        {
                            case '"':
                                tmp |= KATAL_CPP_INCLUDE_SEARCH_IN_BASE;
                            case '<':
                                tmp |= KATAL_CPP_INCLUDE_IN_STRING;
                                break;
                            default:
                                tmp++;
                                break;
                        }
                    }
                }
                else if (opt & KATAL_CPP_IN_DEFINE)
                {
                    /* handle #define */
                    /* stub */
                }
                else if (opt & KATAL_CPP_IN_IF)
                {
                    depth++;
                    /* stub */
                }
                else if (opt & KATAL_CPP_IN_IFDEF)
                {
                    depth++;
                    /* stub */
                }
                else if (opt & KATAL_CPP_IN_ELIF)
                {
                    /* stub */
                }
                else if (opt & KATAL_CPP_IN_ELSE)
                {
                    if (depth == 1)
                    {
                        opt ^= KATAL_CPP_CONDITIONAL_SKIPPING;
                    }
                }
                else if (opt & KATAL_CPP_IN_ENDIF)
                {
                    depth--;
                }
                else
                {
                    /* don't know which instruction we're in just yet */

                    switch (b[i])
                    {
                        case '\\':
                            /* need to handle this for situations where newlines
                             * are being escaped in instructions */
                            opt |= KATAL_CPP_IN_ESCAPE;
                            break;
                        case '\n':
                            if (!opt & KATAL_CPP_IN_ESCAPE)
                            {
                                opt ^= KATAL_CPP_IN_INSTRUCTION;
                            }
                        case ' ':
                        case '\v':
                        case '\t':
                            /* need to know which instruction we found right
                             * right about now. */

                            if ((tmp == 2) && (opt & KATAL_CPP_PROBABLY_IF))
                            {
                                opt ^= KATAL_CPP_PROBABLY_IF
                                     | KATAL_CPP_IN_IF;
                            }
                            else if ((tmp == 5) &&
                                     (opt & KATAL_CPP_PROBABLY_IF))
                            {
                                opt ^= KATAL_CPP_PROBABLY_IF
                                     | KATAL_CPP_IN_IFDEF;
                            }
                            else if ((tmp == 7) &&
                                     (opt & KATAL_CPP_PROBABLY_INCLUDE))
                            {
                                opt ^= KATAL_CPP_PROBABLY_INCLUDE
                                     | KATAL_CPP_IN_INCLUDE;
                                tmp = 0;
                            }
                            else if ((tmp == 4) &&
                                     (opt & KATAL_CPP_PROBABLY_ELIF))
                            {
                                opt ^= KATAL_CPP_PROBABLY_ELIF
                                     | KATAL_CPP_IN_ELIF;
                            }
                            else if ((tmp == 4) &&
                                     (opt & KATAL_CPP_PROBABLY_ELSE))
                            {
                                opt ^= KATAL_CPP_PROBABLY_ELSE
                                     | KATAL_CPP_IN_ELSE;
                            }
                            else if ((tmp == 5) &&
                                     (opt & KATAL_CPP_PROBABLY_ENDIF))
                            {
                                opt ^= KATAL_CPP_PROBABLY_ENDIF
                                     | KATAL_CPP_IN_ENDIF;
                            }
                            else if ((tmp == 6) &&
                                     (opt & KATAL_CPP_PROBABLY_DEFINE))
                            {
                                opt ^= KATAL_CPP_PROBABLY_DEFINE
                                     | KATAL_CPP_IN_DEFINE;
                                tmp = 0;
                                d->tstring = (const char *)0;
                            }
                            else
                            {
                                tmp++;
                                goto unhandled_instruction;
                            }

                            break;

                        case 'c':
                            if ((tmp == 2) &&
                                (opt & KATAL_CPP_PROBABLY_INCLUDE))
                            {
                                tmp++;
                            }
                            else
                            {
                                goto unhandled_instruction;
                            }

                            break;

                        case 'd':
                            if ((tmp == 5) &&
                                (opt & KATAL_CPP_PROBABLY_INCLUDE))
                            {
                                tmp++;
                            }
                            else if ((tmp == 2) &&
                                     (opt & KATAL_CPP_PROBABLY_IF))
                            {
                                tmp++;
                            }
                            else if ((tmp == 2) &&
                                     (opt & KATAL_CPP_PROBABLY_ENDIF))
                            {
                                tmp++;
                            }
                            else
                            {
                                goto unhandled_instruction;
                            }

                            break;

                        case 'e':
                            if ((tmp == 6) &&
                                (opt & KATAL_CPP_PROBABLY_INCLUDE))
                            {
                                tmp++;
                            }
                            else if ((tmp == 3) &&
                                     (opt & KATAL_CPP_PROBABLY_IF))
                            {
                                tmp++;
                            }
                            else if ((tmp == 3) &&
                                     (opt & KATAL_CPP_PROBABLY_ELSE))
                            {
                                tmp++;
                            }
                            else if ((tmp == 1) &&
                                     (opt & KATAL_CPP_PROBABLY_DEFINE))
                            {
                                tmp++;
                            }
                            else if ((tmp == 5) &&
                                     (opt & KATAL_CPP_PROBABLY_DEFINE))
                            {
                                tmp++;
                            }
                            else
                            {
                                goto unhandled_instruction;
                            }

                            break;

                        case 'f':
                            if ((tmp == 1) &&
                                (opt & KATAL_CPP_PROBABLY_INCLUDE))
                            {
                                opt ^= KATAL_CPP_PROBABLY_INCLUDE
                                     | KATAL_CPP_PROBABLY_IF;
                                tmp++;
                            }
                            else if ((tmp == 4) &&
                                     (opt & KATAL_CPP_PROBABLY_IF))
                            {
                                tmp++;
                            }
                            else if ((tmp == 3) &&
                                     (opt & KATAL_CPP_PROBABLY_ELIF))
                            {
                                tmp++;
                            }
                            else if ((tmp == 4) &&
                                     (opt & KATAL_CPP_PROBABLY_ENDIF))
                            {
                                tmp++;
                            }
                            else if ((tmp == 2) &&
                                     (opt & KATAL_CPP_PROBABLY_DEFINE))
                            {
                                tmp++;
                            }
                            else
                            {
                                goto unhandled_instruction;
                            }

                            break;

                        case 'i':
                            if ((tmp == 2) &&
                                (opt & KATAL_CPP_PROBABLY_ELSE))
                            {
                                opt ^= KATAL_CPP_PROBABLY_ELSE
                                     | KATAL_CPP_PROBABLY_ELIF;
                                tmp++;
                            }
                            else if ((tmp == 3) && 
                                     (opt & KATAL_CPP_PROBABLY_ENDIF))
                            {
                                tmp++;
                            }
                            else if ((tmp == 3) &&
                                     (opt & KATAL_CPP_PROBABLY_DEFINE))
                            {
                                tmp++;
                            }
                            else
                            {
                                goto unhandled_instruction;
                            }

                            break;

                        case 'l':
                            if ((tmp == 3) &&
                                (opt & KATAL_CPP_PROBABLY_INCLUDE))
                            {
                                tmp++;
                            }
                            else if ((tmp == 1) &&
                                     (opt & KATAL_CPP_PROBABLY_ENDIF))
                            {
                                opt ^= KATAL_CPP_PROBABLY_ENDIF
                                     | KATAL_CPP_PROBABLY_ELSE;
                                tmp++;
                            }
                            else
                            {
                                goto unhandled_instruction;
                            }

                            break;

                        case 'n':
                            if ((tmp == 1) &&
                                (opt & KATAL_CPP_PROBABLY_INCLUDE))
                            {
                                tmp++;
                            }
                            else if ((tmp == 1) &&
                                     (opt & KATAL_CPP_PROBABLY_ENDIF))
                            {
                                tmp++;
                            }
                            else if ((tmp == 4) &&
                                     (opt & KATAL_CPP_PROBABLY_DEFINE))
                            {
                                tmp++;
                            }
                            else
                            {
                                goto unhandled_instruction;
                            }

                            break;

                        case 's':
                            if ((tmp == 2) &&
                                (opt & KATAL_CPP_PROBABLY_ELSE))
                            {
                                tmp++;
                            }
                            else
                            {
                                goto unhandled_instruction;
                            }

                            break;

                        case 'u':
                            if ((tmp == 4) &&
                                (opt & KATAL_CPP_PROBABLY_INCLUDE))
                            {
                                tmp++;
                            }
                            else
                            {
                                goto unhandled_instruction;
                            }

                            break;

                        default:
                        unhandled_instruction:
                            /* unsupported instruction, just copy verbatim
                             * (still gonna strip possible whitespace before the
                             * instruction identifier, though) */

                            if (opt & KATAL_CPP_PROBABLY_IF)
                            {
                                io_collect (d->out, "#ifdef ", 1 + tmp);
                            }
                            else if (opt & KATAL_CPP_PROBABLY_INCLUDE)
                            {
                                io_collect (d->out, "#include ", 1 + tmp);
                            }
                            else if (opt & KATAL_CPP_PROBABLY_ELSE)
                            {
                                io_collect (d->out, "#else ", 1 + tmp);
                            }
                            else if (opt & KATAL_CPP_PROBABLY_ELIF)
                            {
                                io_collect (d->out, "#elif ", 1 + tmp);
                            }
                            else if (opt & KATAL_CPP_PROBABLY_ENDIF)
                            {
                                io_collect (d->out, "#endif ", 1 + tmp);
                            }
                            else if (opt & KATAL_CPP_PROBABLY_DEFINE)
                            {
                                io_collect (d->out, "#define ", 1 + tmp);
                            }

                            opt &= ~(KATAL_CPP_IN_INSTRUCTION |
                                     KATAL_CPP_PROBABLY_IF |
                                     KATAL_CPP_PROBABLY_ELSE |
                                     KATAL_CPP_PROBABLY_ELIF |
                                     KATAL_CPP_PROBABLY_ENDIF |
                                     KATAL_CPP_PROBABLY_DEFINE |
                                     KATAL_CPP_PROBABLY_INCLUDE |
                                     KATAL_CPP_CONDITIONAL_SKIPPING);
                            break;
                    }
                }

                opt &= ~KATAL_CPP_IN_ESCAPE; /* clear the escape flag */

                continue;
            }
            else
            {
                if (opt & KATAL_CPP_IN_INCLUDE)
                {
                    opt ^= KATAL_CPP_IN_INCLUDE;
                }
                else if (opt & KATAL_CPP_IN_DEFINE)
                {
                    opt ^= KATAL_CPP_IN_DEFINE;
                    /* stub */
                }
                else if (opt & KATAL_CPP_IN_IF)
                {
                    opt ^= KATAL_CPP_IN_IF;
                    depth++;
                }
                else if (opt & KATAL_CPP_IN_IFDEF)
                {
                    opt ^= KATAL_CPP_IN_IFDEF;
                    depth++;
                }
                else if (opt & KATAL_CPP_IN_ELIF)
                {
                    opt ^= KATAL_CPP_IN_ELIF;
                    /* stub */
                }
                else if (opt & KATAL_CPP_IN_ELSE)
                {
                    opt ^= KATAL_CPP_IN_ELSE;
                    /* stub */
                }
                else if (opt & KATAL_CPP_IN_ENDIF)
                {
                    opt ^= KATAL_CPP_IN_ENDIF;
                    depth--;
                }
            }

            if (opt & KATAL_CPP_IN_ESCAPE)
            {
                /* we don't actually parse escapes in the preprocessor... */

                opt ^= KATAL_CPP_IN_ESCAPE;

                io_collect (d->out, b + i, 1);

                continue;
            }

            if (opt & KATAL_CPP_IN_STRING)
            {
                switch (b[i])
                {
                    case '"':
                        opt ^= KATAL_CPP_IN_STRING | KATAL_CPP_POST_STRING;
                        /* note: termination of the string is delayed until we
                         * know if a string may be following next */
                        break;
                    case '\\':
                        opt |= KATAL_CPP_IN_ESCAPE;
                    default:
                        io_collect (d->out, b + i, 1);
                        break;
                }

                continue;
            }

            if (opt & KATAL_CPP_POST_STRING)
            {
                switch (b[i])
                {
                    case '\n':
                        opt |= KATAL_CPP_POST_NEWLINE;
                        break;
                    case ' ':
                    case '\t':
                    case '\v':
                    case 0:
                        opt &= ~KATAL_CPP_POST_NEWLINE;
                        /* ignore whitespace (and strip it) */
                        break;
                    case '\"':
                        opt = (opt &
                               ~(KATAL_CPP_POST_NEWLINE|KATAL_CPP_POST_STRING))
                              | KATAL_CPP_IN_STRING;
                        break;
                    default:
                        /* terminate the string properly */
                        opt ^= KATAL_CPP_POST_STRING;
                        io_collect (d->out, "\"", 1);
                        goto parse_buffer_element;
                }

                continue;
            }

          parse_buffer_element:
            switch (b[i])
            {
                case '#':
                    if (opt & KATAL_CPP_POST_NEWLINE)
                    {
                        opt = (opt & ~KATAL_CPP_POST_NEWLINE)
                                   | KATAL_CPP_IN_INSTRUCTION;
                        tmp = 0;
                        goto skip_collect;
                    }
                    break;
                case '"':
                    opt = (opt & ~KATAL_CPP_POST_NEWLINE)
                               | KATAL_CPP_IN_STRING;
                    break;
                case '\n':
                    opt |= KATAL_CPP_POST_NEWLINE;
                    break;
            }

            io_collect (d->out, b + i, 1);

          skip_collect: ;
        }

        in->position = i;
        d->options   = opt;
        d->tmp       = tmp;
        d->depth     = depth;

        io_commit (d->out);

        if ((in->position == in->length) &&
            (d->options & KATAL_CPP_MAY_CLOSE))
        {
            if (d->on_end_of_input != (void *)0)
            {
                d->on_end_of_input (d->aux);
            }
        
#warning on_cpp_read() is not freeing resources as well as it should just yet.

            free_pool_mem (aux);

            io_close (in);
        }
    }
}

static void on_cpp_close (struct io *in, void *aux)
{
    struct ppdata *d = (struct ppdata *)aux;
    struct io *tin = io_open_special();

    d->options |= KATAL_CPP_MAY_CLOSE;
    d->in = tin;

    io_write (tin, in->buffer + in->position, in->length - in->position);

    on_cpp_read (tin, aux);
}

void katal_c_preprocess
    (unsigned int options, struct io *in, struct io *out,
     const char **include, const char *base, const char **defines,
     void (*on_end_of_input)(void *),
     void (*on_notice)(enum katal_notice, const char *, void *),
     void *aux)
{
    struct memory_pool pool = MEMORY_POOL_INITIALISER (sizeof (struct ppdata));
    struct ppdata *d = get_pool_mem (&pool);

    d->options         = options;
    d->in              = in;
    d->out             = out;
    d->include         = include;
    d->base            = base;
    d->defines         = defines;
    d->on_end_of_input = on_end_of_input;
    d->on_notice       = on_notice;
    d->depth           = 0;
    d->aux             = aux;

    multiplex_add_io (in, on_cpp_read, on_cpp_close, (void *)d);
}

void katal_c_preprocess_file
    (unsigned int options, const char *file, struct io *out,
     const char **include, const char **defines,
     void (*on_end_of_input)(void *),
     void (*on_notice)(enum katal_notice, const char *, void *),
     void *aux)
{
    unsigned long last_path_delim_at = 0, i = 0;
    const char *path = (const char *)0;

    while (file[i] != 0)
    {
        switch (file[i])
        {
            case '/':
            case '\\':
                last_path_delim_at = i;
        }
 
        i++;
    }

    if (last_path_delim_at != 0)
    {
        unsigned long len = last_path_delim_at + 2;
        char *tpath = aalloc (len);

        for (i = 0; i <= last_path_delim_at; i++)
        {
            tpath[i] = file[i];
        }

        tpath[i] = 0;

        path = str_immutable (tpath);

        afree (len, tpath);
    }

    katal_c_preprocess (options, io_open_read (file), out,
                        include, path, defines, on_end_of_input, on_notice,
                        aux);
}

