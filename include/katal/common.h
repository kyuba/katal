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

extern const char *katal_include_directories[];

#define KATAL_PREPROCESS_STRIP_COMMENTS   (1 << 0)
#define KATAL_PREPROCESS_STRIP_WHITESPACE (1 << 1)

enum katal_return_value
{
    krv_ok,
    krv_invalid_token,
    krv_incomplete
};

#define KATAL_TOKEN_FLAG_HAVE_NEXT        (1 << 31)
#define KATAL_TOKEN_FLAG_HAVE_PAYLOAD_1   (1 << 30)
#define KATAL_TOKEN_FLAG_HAVE_PAYLOAD_2   (1 << 29)
#define KATAL_TOKEN_FLAG_HAVE_PAYLOAD_3   (1 << 28)

enum katal_token_type
{
    /* dummies/informational */
    ktt_none,
    ktt_end_of_file,
    ktt_whitespace,

    /* tokens for symbols */
    ktt_question_mark,
    ktt_colon,
    ktt_hash,
    ktt_bang,
    ktt_asterisk,
    ktt_plus,
    ktt_minus,
    ktt_slash,
    ktt_ampersand,
    ktt_equals,
    ktt_comma,
    ktt_dot,
    ktt_semicolon,
    ktt_pipe,
    ktt_circumflex,
    ktt_tilde,
    ktt_percent,

    ktt_opening_parenthesis, /* () */
    ktt_closing_parenthesis,
    ktt_opening_brace, /* {} */
    ktt_closing_brace,
    ktt_opening_bracket, /* [] */
    ktt_closing_bracket,
    ktt_opening_angle_bracket, /* <> */
    ktt_closing_angle_bracket,

    /* extended symbolic tokens (keywords) */
    ktt_void,
    ktt_struct,
    ktt_union,
    ktt_enum,
    ktt_typedef,
    ktt_unsigned,
    ktt_signed,
    ktt_short,
    ktt_long,
    ktt_char,
    ktt_int,
    ktt_double,
    ktt_float,
    ktt_return,
    ktt_const,
    ktt_volatile,
    ktt_static,
    ktt_extern,
    ktt_auto,
    ktt_register,
    ktt_do,
    ktt_while,
    ktt_for,
    ktt_continue,
    ktt_break,
    ktt_if,
    ktt_else,
    ktt_switch,
    ktt_case,
    ktt_default,
    ktt_goto,
    ktt_sizeof,

    ktt_inline,
    ktt_complex,

    ktt_typeof,

    /* (mostly) logical tokens */
    ktt_comment,
    ktt_integer,
    ktt_integer_signed,
    ktt_floating_point,
    ktt_string,
    ktt_character_literal,
    ktt_symbol,
    ktt_block,
    ktt_begin_block,
    ktt_end_block,
    ktt_assign,
    ktt_arithmetic_add,
    ktt_arithmetic_subtract,
    ktt_arithmetic_divide,
    ktt_arithmetic_multiply,
    ktt_arithmetic_modulo,
    ktt_shift_left,
    ktt_shift_right,
    ktt_arithmetic_add_and_assign,
    ktt_arithmetic_subtract_and_assign,
    ktt_arithmetic_divide_and_assign,
    ktt_arithmetic_multiply_and_assign,
    ktt_arithmetic_modulo_and_assign,
    ktt_shift_left_and_assign,
    ktt_shift_right_and_assign,
    ktt_lesser_than,
    ktt_greater_than,
    ktt_equality,
    ktt_unequality,
    ktt_end_of_expression,
    ktt_group,
    ktt_group_begin,
    ktt_group_end,
    ktt_logical_or,
    ktt_logical_and,
    ktt_logical_xor,
    ktt_logical_negation,
    ktt_bitwise_or,
    ktt_bitwise_and,
    ktt_bitwise_xor,
    ktt_bitwise_negation,
    ktt_bitwise_or_and_assign,
    ktt_bitwise_and_and_assign,
    ktt_bitwise_xor_and_assign,
    ktt_bitwise_negation_and_assign,
    ktt_trinear_if_condition_mark,
    ktt_trinear_if_branch_delimiter,
    ktt_shebang,
    ktt_pointer_to,
    ktt_reference_of,
    ktt_dereference_of,
    ktt_increment,
    ktt_decrement,

    /* purely logical tokens */
    ktt_declaration,
    ktt_definition,
    ktt_type,
    ktt_variable,
    ktt_literal,
    ktt_function_prototype,
    ktt_typecast,
    ktt_label
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
    unsigned long flags;
    union katal_token_payload payload[];
};

struct katal_token_with_next
{
    enum katal_token_type type;
    unsigned long flags;
    struct katal_token *next;
    union katal_token_payload payload[];
};

void katal_initialise ( void );

struct katal_token *katal_token_immutable
    (enum katal_token_type type, unsigned long long flags,
     struct katal_token *next,
     union katal_token_payload *primus,
     union katal_token_payload *secundus,
     union katal_token_payload *tertius);

void katal_token_free_all ( void );

int katal_token_equalp (struct katal_token *a, struct katal_token *b);

#endif

