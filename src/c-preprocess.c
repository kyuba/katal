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
#include <katal/c.h>

struct ppdata
{
    unsigned int options;
    struct io *out;
    const char **include;
    const char *base;
    const char **defines;
    void (*on_end_of_input)(void *);
    void *aux;
};

static void on_cpp_read (struct io *in, void *aux)
{
    struct ppdata *d = (struct ppdata *)aux;
}

static void on_cpp_close (struct io *in, void *aux)
{
    struct ppdata *d = (struct ppdata *)aux;

    if (d->on_end_of_input != (void *)0)
    {
        d->on_end_of_input (d->aux);
    }

    free_pool_mem (aux);
}

enum katal_return_value katal_c_preprocess
    (unsigned int options, struct io *in, struct io *out,
     const char **include, const char *base, const char **defines,
     void (*on_end_of_input)(void *),
     void *aux)
{
    struct memory_pool pool = MEMORY_POOL_INITIALISER (sizeof (struct ppdata));
    struct ppdata *d = get_pool_mem (&pool);

    d->options         = options;
    d->out             = out;
    d->include         = include;
    d->base            = base;
    d->defines         = defines;
    d->on_end_of_input = on_end_of_input;
    d->aux             = aux;

    multiplex_add_io (in, on_cpp_read, on_cpp_close, (void *)d);
}

