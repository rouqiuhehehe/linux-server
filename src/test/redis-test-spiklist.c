//
// Created by 115282 on 2023/8/8.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#define MAX_RANG 10000000
#define TIME_USE(begin, end) (((end).tv_sec - (begin).tv_sec) * 1000 + ((end).tv_usec - (begin).tv_usec) / 1000)

#define ZSKIPLIST_MAXLEVEL 32
#define ZSKIPLIST_P 0.25
#define MIN(a, b) ((a) < (b) ? (a) : (b))
typedef char *sds;
typedef struct zskiplist
{
    struct zskiplistNode *header, *tail;
    unsigned long length;
    int level;
} zskiplist;
typedef struct zskiplistNode
{
    sds ele;
    double score;
    struct zskiplistNode *backward;
    struct zskiplistLevel
    {
        struct zskiplistNode *forward;
        unsigned long span;
    } level[];
} zskiplistNode;
zskiplistNode *zslCreateNode (int level, double score, sds ele)
{
    zskiplistNode *zn =
        malloc(sizeof(*zn) + level * sizeof(struct zskiplistLevel));
    zn->score = score;
    zn->ele = ele;
    return zn;
}
zskiplist *zslCreate (void)
{
    int j;
    zskiplist *zsl;

    zsl = malloc(sizeof(*zsl));
    zsl->level = 1;
    zsl->length = 0;
    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL, 0, NULL);
    for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++)
    {
        zsl->header->level[j].forward = NULL;
        zsl->header->level[j].span = 0;
    }
    zsl->header->backward = NULL;
    zsl->tail = NULL;
    return zsl;
}
int zslRandomLevel (void)
{
    static const int threshold = ZSKIPLIST_P * RAND_MAX;
    int level = 1;
    while (random() < threshold)
        level += 1;
    return (level < ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

zskiplistNode *zslInsert (zskiplist *zsl, double score, sds ele)
{
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    unsigned long rank[ZSKIPLIST_MAXLEVEL];
    int i, level;

    x = zsl->header;
    for (i = zsl->level - 1; i >= 0; i--)
    {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (zsl->level - 1) ? 0 : rank[i + 1];
        while (x->level[i].forward &&
            (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score &&
                    memcmp(
                        x->level[i].forward->ele, ele, MIN(strlen(x->level[i].forward->ele),
                            strlen(ele))) < 0)))
        {
            rank[i] += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    /* we assume the element is not already inside, since we allow duplicated
     * scores, reinserting the same element should never happen since the
     * caller of zslInsert() should test in the hash table if the element is
     * already inside or not. */
    level = zslRandomLevel();
    if (level > zsl->level)
    {
        for (i = zsl->level; i < level; i++)
        {
            rank[i] = 0;
            update[i] = zsl->header;
            update[i]->level[i].span = zsl->length;
        }
        zsl->level = level;
    }
    x = zslCreateNode(level, score, ele);
    for (i = 0; i < level; i++)
    {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;

        /* update span covered by update[i] as x is inserted here */
        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }

    /* increment span for untouched levels */
    for (i = level; i < zsl->level; i++)
    {
        update[i]->level[i].span++;
    }

    x->backward = (update[0] == zsl->header) ? NULL : update[0];
    if (x->level[0].forward)
        x->level[0].forward->backward = x;
    else
        zsl->tail = x;
    zsl->length++;
    return x;
}

static long getRandomSds ()
{
    return random() % MAX_RANG;
}
int main ()
{
    zskiplist *list = zslCreate();
    int alignof = __alignof__(zskiplist);

    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    long sds;
    for (int i = 0; i < MAX_RANG; ++i)
    {
        char *str = malloc(alignof << 2);
        sds = getRandomSds();
        sprintf(str, "%ld%c", sds, '\0');
        zslInsert(list, sds, str);
    }
    gettimeofday(&end, NULL);

    printf("time used : %ld-----\n", TIME_USE(begin, end));

    zskiplistNode *node = list->header->level[0].forward;
    while (node->level[0].forward)
    {
        node = node->level[0].forward;
    }
    printf("%s\t", node->ele);
    return 0;
}