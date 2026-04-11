#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "rtrie.h"
#include "consensus.h"
#include "colors.h"

// root of the trie
static RTrieNode *ROOT = NULL;

// create a node
static RTrieNode *make_node(const char *path) {
    RTrieNode *n = malloc(sizeof(RTrieNode));
    n->path = strdup(path);
    n->perm = P_READ;
    n->color = 'R';
    n->left = n->right = n->parent = NULL;
    return n;
}

// insert a node
void rtrie_insert(const char *path) {
    RTrieNode *n = make_node(path);

    if (!ROOT) {
        ROOT = n;
        ROOT->color = 'B';
        return;
    }

    RTrieNode *cur = ROOT, *parent = NULL;
    while (cur) {
        parent = cur;
        if (strcmp(path, cur->path) < 0)
            cur = cur->left;
        else
            cur = cur->right;
    }

    n->parent = parent;
    if (strcmp(path, parent->path) < 0)
        parent->left = n;
    else
        parent->right = n;
}

// find a node
RTrieNode *rtrie_find(const char *path) {
    RTrieNode *cur = ROOT;
    while (cur) {
        int cmp = strcmp(path, cur->path);
        if (cmp == 0) return cur;
        cur = (cmp < 0 ? cur->left : cur->right);
    }
    return NULL;
}

// request permission
int rtrie_request_perm(RTrieNode *n, PermType p) {
    if (!n) return 0;
    if (p == P_EXEC && !consensus_ready())
        return 0;
    return 1;
}

// apply permission
void rtrie_apply_perm(RTrieNode *n, PermType p) {
    if (!n) return;
    n->perm = p;
    consensus_clear();
}

// print
static void print_node(RTrieNode *n) {
    printf("%sNODE%s(%s): perm=%d color=%c\n",
           C_CYAN, C_RESET, n->path, n->perm, n->color);
}

static void print_tree(RTrieNode *n) {
    if (!n) return;
    print_tree(n->left);
    print_node(n);
    print_tree(n->right);
}

void rtrie_print() {
    print_tree(ROOT);
}

// scan folder for *.txt files
void rtrie_scan_txt(const char *folder) {
    DIR *d = opendir(folder);
    if (!d) {
        printf("Folder not found: %s\n", folder);
        return;
    }

    struct dirent *e;
    while ((e = readdir(d))) {
        const char *name = e->d_name;
        size_t len = strlen(name);

        if (len > 4 && strcmp(name + len - 4, ".txt") == 0) {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s/%s", folder, name);
            rtrie_insert(buf);
        }
    }

    closedir(d);
}
