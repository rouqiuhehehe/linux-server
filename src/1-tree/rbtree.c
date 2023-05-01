//
// Created by Administrator on 2023/4/11.
//

#include <stdio.h>
#include <stdlib.h>

#define RED 1
#define BLACK 2

typedef int KEY_TYPE;

typedef struct _rbTreeNode // NOLINT(bugprone-reserved-identifier)
{
    KEY_TYPE key;
    void *value;

    struct _rbTreeNode *left;
    struct _rbTreeNode *right;
    struct _rbTreeNode *parent;

    unsigned char color;
} RBTreeNode;

typedef int (*CompareFun) (RBTreeNode *x, RBTreeNode *y);

typedef struct _rbTree // NOLINT(bugprone-reserved-identifier)
{
    RBTreeNode *root;
    RBTreeNode *nil;
    CompareFun compareFun;
} RBTree;

static void rbTreeLeftRotate (RBTree *tree, RBTreeNode *node)
{
    RBTreeNode *right = node->right;

    // node 的parent指向right
    // right 的left指向node
    // node 的right指向right的left
    // 1
    if (node->parent == tree->nil)
        tree->root = right;
    else if (node == node->parent->left)
        node->parent->left = right;
    else
        node->parent->right = right;

    node->parent = right;

    // 3
    node->right = right->left;
    if (right->left != tree->nil)
        right->left->parent = node;
    // 2
    right->left = node;
}

static void rbTreeRightRotate (RBTree *tree, RBTreeNode *node)
{
    RBTreeNode *left = node->left;

    // node 的parent指向left
    // left 的right指向node
    // node 的left指向left的right

    if (node->parent == tree->nil)
        tree->root = left;
    else if (node == node->parent->left)
        node->parent->left = left;
    else
        node->parent->right = left;

    node->parent = left;

    node->left = left->right;
    if (left->right != tree->nil)
        left->right->parent = node;

    left->right = node;
}

static void rbTreeNodeDyeing (RBTreeNode *node, RBTreeNode *uncle)
{
    uncle->color = BLACK;
    node->parent->color = BLACK;
    node->parent->parent->color = RED;
}
static void rbTreeInsertFixup (RBTree *tree, RBTreeNode *node)
{
    // 如果一个节点是红的，则他两个子节点都是黑的
    // 对于每个节点，从该节点到其子孙节点的所有路径上包含相同数目的黑节点

    // 当前node为红色，所以node->parent为黑色则不需要调整
    RBTreeNode *uncle;
    while (node->parent->color == RED)
    {
        if (node->parent == node->parent->parent->left)
        {
            uncle = node->parent->parent->right;

            // 如果叔父节点为红色，则只需重新染色即可
            if (uncle->color == RED)
            {
                rbTreeNodeDyeing(node, uncle);

                // 回溯，每一次循环都要保证node是红色
                node = node->parent->parent;
            }
            else
            {
                // 如果node是parent的右节点，需要先将node指向parent，进行左旋，转为左倾后右旋
                if (node == node->parent->right)
                {
                    node = node->parent;
                    rbTreeLeftRotate(tree, node);
                }

                node->parent->color = BLACK;
                node->parent->parent->color = RED;

                rbTreeRightRotate(tree, node->parent->parent);
            }
        }
        else
        {
            uncle = node->parent->parent->left;

            if (uncle->color == RED)
            {
                rbTreeNodeDyeing(node, uncle);

                node = node->parent->parent;
            }
            else
            {
                if (node == node->parent->left)
                {
                    node = node->parent;
                    rbTreeRightRotate(tree, node);
                }

                node->parent->color = BLACK;
                node->parent->parent->color = RED;

                rbTreeLeftRotate(tree, node->parent->parent);
            }
        }
    }
}

void rbTreeInsert (RBTree *tree, RBTreeNode *node)
{
    node->left = tree->nil;
    node->right = tree->nil;
    node->color = RED;
    if (tree->root == tree->nil)
    {
        node->color = BLACK;
        tree->root = node;
        return;
    }

    RBTreeNode *root = tree->root;
    RBTreeNode *parent = tree->nil;

    int ret;
    while (root != tree->nil)
    {
        parent = root;
        ret = tree->compareFun(root, node);
        if (ret > 0)
            root = root->right;
        else
            root = root->left;
    }

    node->parent = parent;
    if (ret > 0)
        parent->right = node;
    else
        parent->left = node;

    rbTreeInsertFixup(tree, node);
}

int rbTreeInit (RBTree *tree, CompareFun compareFun)
{
    tree->nil = malloc(sizeof(RBTreeNode));
    tree->nil->color = BLACK;

    if (tree->nil == NULL)
        return -1;
    tree->root = tree->nil;
    tree->compareFun = compareFun;

    return 0;
}

int rbTreeDestroy (RBTree *tree)
{
    free(tree->nil);

    return 0;
}