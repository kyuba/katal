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

#ifndef LIBKATAL_C_H
#define LIBKATAL_C_H

#include <curie/io.h>
#include <katal/common.h>

void katal_c_preprocess
    (unsigned int options, struct io *in, struct io *out,
     const char **include, const char *base, const char **defines,
     void (*on_end_of_input)(void *),
     void (*on_notice)(enum katal_notice, const char *, void *),
     void *aux);

void katal_c_preprocess_file
    (unsigned int options, const char *file, struct io *out,
     const char **include, const char **defines,
     void (*on_end_of_input)(void *),
     void (*on_notice)(enum katal_notice, const char *, void *),
     void *aux);

struct katal_token *katal_c_get_token
    (unsigned int options, struct io *in);

enum katal_return_value katal_c_parse
    (unsigned int options, struct io *in, struct katal_token **out);

#endif

