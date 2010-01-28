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
#include <curie/hash.h>
#include <curie/tree.h>
#include <katal/common.h>

static struct tree token_tree = TREE_INITIALISER;

struct katal_token *katal_token_immutable
    (enum katal_token_type type, unsigned long flags,
     struct katal_token *next, union katal_token_payload *primus,
     union katal_token_payload *secundus, union katal_token_payload *tertius)
{
    unsigned int num_tokens = 0, size = 0, i, j, *payloadbase, *copybase;
    struct katal_token *rv;
    int_pointer hash;
    struct tree_node *node;

    if (primus != (union katal_token_payload *)0)
    {
        flags |= KATAL_TOKEN_FLAG_HAVE_PAYLOAD_1;
        num_tokens++;
    }

    if (secundus != (union katal_token_payload *)0)
    {
        flags |= KATAL_TOKEN_FLAG_HAVE_PAYLOAD_2;
        num_tokens++;
    }

    if (tertius != (union katal_token_payload *)0)
    {
        flags |= KATAL_TOKEN_FLAG_HAVE_PAYLOAD_3;
        num_tokens++;
    }

    if (next == (struct katal_token *)0)
    {
        rv = (struct katal_token *)
             aalloc (size = (sizeof (struct katal_token) +
                            num_tokens * sizeof (union katal_token_payload)));

        payloadbase = (unsigned int *)(rv->payload);
    }
    else
    {
        struct katal_token_with_next *rvt;

        rvt = (struct katal_token_with_next *)
              aalloc (size = (sizeof (struct katal_token) +
                             num_tokens * sizeof (union katal_token_payload)));

        payloadbase = (unsigned int *)(rvt->payload);

        rvt->flags |= KATAL_TOKEN_FLAG_HAVE_NEXT;
        rvt->next   = next;

        rv = (struct katal_token *)rvt;
    }

    for (i = 0; i < num_tokens; i++)
    {
        switch (i)
        {
            case 0: copybase = (unsigned int *)primus;   break;
            case 1: copybase = (unsigned int *)secundus; break;
            case 2: copybase = (unsigned int *)tertius;  break;
        }

        for (j = 0;
             j < (sizeof (union katal_token_payload) / sizeof (unsigned int));
             j++)
        {
            *payloadbase = *copybase;
            payloadbase++;
            copybase++;
        }
    }

    rv->type   = type;
    rv->flags |= flags;

    hash = hash_murmur2_pt ((const void *)rv, size, 0);

    node = tree_get_node (&token_tree, hash);

    if (node != (struct tree_node *)0)
    {
        afree (size, (void *)rv);
        rv = (struct katal_token *)node_get_value (node);
    }
    else
    {
        tree_add_node_value (&token_tree, hash, (void *)rv);
    }

    return rv;
}

