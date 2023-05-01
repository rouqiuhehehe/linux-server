//
// Created by Administrator on 2023/4/20.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#define BTREE_M 6

typedef int KEY_TYPE;

typedef struct _BTreeNode
{
    // 有多少个节点
    int keyNum;
    // 是否为叶子节点
    char isLeaf;
    struct _BTreeNode **children;
    KEY_TYPE *keys;
} BTreeNode;

typedef struct
{
    BTreeNode *root;
    // m阶B树
    // 每个节点最多拥有m个子树，根节点至少有2个子树，其余子节点至少拥有m/2个子树
    int m;
} BTree;

static BTreeNode *bTreeCreateNode (int m, char isLeaf)
{
    BTreeNode *node = calloc(1, sizeof(BTreeNode));
    assert(node != NULL);

    node->isLeaf = isLeaf;
    node->keys = calloc(m - 1, sizeof(KEY_TYPE));
    node->children = calloc(m, sizeof(BTreeNode *));

    return node;
}

static void bTreeNodeDestroy (BTreeNode *node)
{
    assert(node);

    free(node->children);
    free(node->keys);
    free(node);
}

void bTreeCreate (BTree *tree, int m)
{
    tree->root = bTreeCreateNode(m, 1);
    tree->m = m;
}

// > m -1 时分裂
void bTreeSplitChild (BTree *bTree, BTreeNode *node, int index)
{
    BTreeNode *splitNode = node->children[index];
    BTreeNode *rightNode = bTreeCreateNode(bTree->m, splitNode->isLeaf);

    int num = (int)ceil(bTree->m / 2) - 1;
    rightNode->keyNum = num;

    int i;
    for (i = 0; i < num; ++i)
        rightNode->keys[i] = splitNode->keys[i + num];
    // 如果是非叶子节点，还需复制children
    if (splitNode->isLeaf == 0)
        for (i = 0; i < num + 1; ++ i)
        {
            rightNode->children[i] = splitNode->children[i + num];
        }

    for (i = splitNode->keyNum; i >= index + 1; --i)
        splitNode->children[i + 1] = splitNode->children[i];
    // 移位，保证顺序
    splitNode->children[i + 1] = rightNode;
}

int main ()
{
    setbuf(stdout, 0);

    return 0;
}