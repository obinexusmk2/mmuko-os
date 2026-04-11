#ifndef RTRIE_H
#define RTRIE_H


#include <stddef.h>


// Permission types
typedef enum {
P_READ = 1,
P_WRITE = 2,
P_EXEC = 4
} PermType;


// Node structure
typedef struct RTrieNode {
char *path;
PermType perm;
char color;
struct RTrieNode *left;
struct RTrieNode *right;
struct RTrieNode *parent;
} RTrieNode;


void rtrie_insert(const char *path);
void rtrie_print();
RTrieNode *rtrie_find(const char *path);
void rtrie_scan_txt(const char *folder);


int rtrie_request_perm(RTrieNode *n, PermType p);
void rtrie_apply_perm(RTrieNode *n, PermType p);


#endif
