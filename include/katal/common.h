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

#ifndef LIBKATAL_COMMON_H
#define LIBKATAL_COMMON_H

#define KATAL_PREPROCESS_STRIP_COMMENTS   (1 << 0)
#define KATAL_PREPROCESS_STRIP_WHITESPACE (1 << 1)

enum katal_return_value
{
    krv_ok,
    krv_invalid_token,
    krv_incomplete
};

enum katal_token_type
{
    ktt_none,
    ktt_end_of_file,
    ktt_comment,
    ktt_whitespace,
    ktt_integer,
    ktt_integer_signed,
    ktt_floating_point,
    ktt_string,
    ktt_type,
    ktt_variable,
    ktt_symbol,
    ktt_block,
    ktt_begin_block,
    ktt_end_block,
    ktt_assign,
    ktt_arithmetic_add,
    ktt_arithmetic_subtract,
    ktt_arithmetic_divide,
    ktt_arithmetic_multiply,
    ktt_shift_left,
    ktt_shift_right,
    ktt_arithmetic_add_and_assign,
    ktt_arithmetic_subtract_and_assign,
    ktt_arithmetic_divide_and_assign,
    ktt_arithmetic_multiply_and_assign,
    ktt_shift_left_and_assign,
    ktt_shift_right_and_assign,
    ktt_lesser_than,
    ktt_greater_than,
    ktt_equality,
    ktt_end_of_expression,
    ktt_group,
    ktt_group_begin,
    ktt_group_end,
    ktt_opening_parenthesis,
    ktt_closing_parenthesis,
    ktt_opening_brace,
    ktt_closing_brace,
    ktt_opening_bracket,
    ktt_closing_bracket
};

union katal_token_payload
{
    unsigned long long  integer;
    signed   long long  integer_signed;
    long double         floating_point;
    const char         *string;
    struct kata_token  *token;
};

struct katal_token
{
    enum katal_token_type type;
    unsigned long long flags;

    union katal_token_payload payload;

    struct katal_token *next;
};

struct katal_token *katal_token_immutable
    (enum katal_token_type type, unsigned long long flags,
     union katal_token_payload payload, struct katal_token *next);

#endif

