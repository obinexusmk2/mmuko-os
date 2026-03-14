/*
 * interdependency.c - MMUKO-OS Interdependency Tree System
 * 
 * Implements the interdependency resolution algorithm for the boot sequence.
 * A depends on B, B depends on C, etc. - resolved via topological sort.
 * 
 * Following the riftbridge protocol from github.com/obinexus/riftbridge
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/mmuko_types.h"

/* Static tree state */
static InterdepTree *boot_tree = NULL;
static uint8_t resolution_stack[256];
static int stack_ptr = 0;

/**
 * Create a new interdependency tree
 * Returns: Pointer to initialized tree
 */
InterdepTree* interdep_tree_create(void) {
    InterdepTree *tree = (InterdepTree*)malloc(sizeof(InterdepTree));
    if (!tree) return NULL;
    
    tree->root = NULL;
    tree->node_count = 0;
    tree->resolved_count = 0;
    tree->max_depth = 0;
    
    return tree;
}

/**
 * Destroy interdependency tree and all nodes
 */
void interdep_tree_destroy(InterdepTree *tree) {
    if (!tree) return;
    
    /* Recursive node cleanup would go here */
    /* For boot sector, we keep it simple */
    
    free(tree);
}

/**
 * Create a new interdependency node
 * @id: Node identifier
 * @level: Tree level (ROOT, TRUNK, BRANCH, LEAF)
 * Returns: Pointer to new node
 */
InterdepNode* interdep_node_create(uint8_t id, uint8_t level) {
    InterdepNode *node = (InterdepNode*)malloc(sizeof(InterdepNode));
    if (!node) return NULL;
    
    node->id = id;
    node->level = level;
    node->state = NODE_UNRESOLVED;
    node->dependency_count = 0;
    node->dependencies = NULL;
    node->resolve_func = NULL;
    node->data = NULL;
    
    return node;
}

/**
 * Add dependency to a node
 * @node: The node that depends on @dep
 * @dep: The dependency that must be resolved first
 */
void interdep_add_dependency(InterdepNode *node, InterdepNode *dep) {
    if (!node || !dep) return;
    
    /* Reallocate dependency array */
    InterdepNode **new_deps = realloc(node->dependencies, 
        (node->dependency_count + 1) * sizeof(InterdepNode*));
    if (!new_deps) return;
    
    node->dependencies = new_deps;
    node->dependencies[node->dependency_count] = dep;
    node->dependency_count++;
}

/**
 * Check for circular dependencies using DFS
 * @node: Current node to check
 * @visited: Array of visited node IDs
 * @visiting: Array of nodes currently in recursion stack
 * Returns: true if circular dependency detected
 */
static bool has_circular_dep(InterdepNode *node, bool *visited, bool *visiting) {
    if (!node) return false;
    
    /* If currently visiting, we found a cycle */
    if (visiting[node->id]) return true;
    
    /* If already visited and no cycle, skip */
    if (visited[node->id]) return false;
    
    /* Mark as visiting */
    visiting[node->id] = true;
    
    /* Check all dependencies */
    for (int i = 0; i < node->dependency_count; i++) {
        if (has_circular_dep(node->dependencies[i], visited, visiting)) {
            return true;
        }
    }
    
    /* Mark as visited, remove from visiting */
    visiting[node->id] = false;
    visited[node->id] = true;
    
    return false;
}

/**
 * Resolve a single node and its dependencies
 * @node: Node to resolve
 * Returns: 0 on success, -1 on failure
 */
int interdep_resolve_node(InterdepNode *node) {
    if (!node) return -1;
    
    /* Already resolved */
    if (node->state == NODE_RESOLVED) return 0;
    
    /* Check for resolution in progress (cycle) */
    if (node->state == NODE_RESOLVING) {
        printf("[INTERDEP] Circular dependency detected at node %d\r\n", node->id);
        return -1;
    }
    
    /* Mark as resolving */
    node->state = NODE_RESOLVING;
    
    /* Resolve all dependencies first */
    for (int i = 0; i < node->dependency_count; i++) {
        if (interdep_resolve_node(node->dependencies[i]) != 0) {
            node->state = NODE_FAILED;
            return -1;
        }
    }
    
    /* Execute node resolution function */
    if (node->resolve_func) {
        node->resolve_func(node);
    }
    
    /* Mark as resolved */
    node->state = NODE_RESOLVED;
    
    /* Add to resolution stack for verification */
    if (stack_ptr < 256) {
        resolution_stack[stack_ptr++] = node->id;
    }
    
    printf("[INTERDEP] Node %d (level %d) resolved\r\n", node->id, node->level);
    
    return 0;
}

/**
 * Resolve entire interdependency tree
 * @tree: Tree to resolve
 * Returns: Number of nodes resolved, -1 on error
 */
int interdep_resolve_tree(InterdepTree *tree) {
    if (!tree || !tree->root) return -1;
    
    bool visited[256] = {false};
    bool visiting[256] = {false};
    
    /* Check for circular dependencies first */
    if (has_circular_dep(tree->root, visited, visiting)) {
        printf("[INTERDEP] ERROR: Circular dependency in tree\r\n");
        return -1;
    }
    
    /* Reset stack */
    stack_ptr = 0;
    
    /* Resolve from root */
    if (interdep_resolve_node(tree->root) != 0) {
        return -1;
    }
    
    tree->resolved_count = stack_ptr;
    return stack_ptr;
}

/**
 * Get resolution order array
 * Returns: Pointer to resolution stack, count in stack_ptr
 */
uint8_t* interdep_get_resolution_order(int *count) {
    if (count) *count = stack_ptr;
    return resolution_stack;
}

/**
 * Create standard MMUKO boot tree
 * Tree structure:
 *   ROOT (0)
 *     └── TRUNK (1) - Memory Manager
 *           ├── BRANCH (2) - Interrupt Handler
 *           │     └── LEAF (3) - Timer
 *           ├── BRANCH (4) - Device Manager
 *           │     └── LEAF (5) - Console
 *           └── BRANCH (6) - File System
 *                 └── LEAF (7) - Boot Loader
 */
InterdepTree* mmuko_create_boot_tree(void) {
    InterdepTree *tree = interdep_tree_create();
    if (!tree) return NULL;
    
    /* Create nodes */
    InterdepNode *root = interdep_node_create(0, TREE_ROOT);
    InterdepNode *trunk = interdep_node_create(1, TREE_TRUNK);
    InterdepNode *branch_irq = interdep_node_create(2, TREE_BRANCH);
    InterdepNode *leaf_timer = interdep_node_create(3, TREE_LEAF);
    InterdepNode *branch_dev = interdep_node_create(4, TREE_BRANCH);
    InterdepNode *leaf_console = interdep_node_create(5, TREE_LEAF);
    InterdepNode *branch_fs = interdep_node_create(6, TREE_BRANCH);
    InterdepNode *leaf_boot = interdep_node_create(7, TREE_LEAF);
    
    /* Build dependency tree */
    interdep_add_dependency(root, trunk);
    interdep_add_dependency(trunk, branch_irq);
    interdep_add_dependency(trunk, branch_dev);
    interdep_add_dependency(trunk, branch_fs);
    interdep_add_dependency(branch_irq, leaf_timer);
    interdep_add_dependency(branch_dev, leaf_console);
    interdep_add_dependency(branch_fs, leaf_boot);
    
    tree->root = root;
    tree->node_count = 8;
    tree->max_depth = 3;
    
    return tree;
}

/**
 * Print tree structure for debugging
 */
void interdep_print_tree(InterdepNode *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("Node %d (level %d, state %d)\r\n", 
           node->id, node->level, node->state);
    
    for (int i = 0; i < node->dependency_count; i++) {
        interdep_print_tree(node->dependencies[i], depth + 1);
    }
}
