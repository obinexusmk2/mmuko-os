/*
 * mmuko_types.h - MMUKO-OS Core Type Definitions
 * M for Mike, U for Uniform, K for Kilo, O for Oscar
 * Interdependency Tree Hierarchy Boot System
 */

#ifndef MMUKO_TYPES_H
#define MMUKO_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* NSIGII Trinary Protocol States */
#define NSIGII_YES      0x55    /* 01010101 - Verified */
#define NSIGII_NO       0xAA    /* 10101010 - Failed */
#define NSIGII_MAYBE    0x00    /* 00000000 - Pending */

/* RIFT Header Magic */
#define RIFT_MAGIC_0    'N'
#define RIFT_MAGIC_1    'X'
#define RIFT_MAGIC_2    'O'
#define RIFT_MAGIC_3    'B'
#define RIFT_VERSION    0x01
#define RIFT_CHECKSUM   0xFE

/* MUCO Boot Constants */
#define MUCO_QUBITS     8       /* 8-qubit compass model */
#define MUCO_HALF_SPIN  0x01    /* π/4 rotation unit */
#define MUCO_NOSIGNAL   0x00    /* No signal state */
#define MUCO_NONOISE    0xF0    /* Cleared noise */

/* Interdependency Node States */
#define NODE_UNRESOLVED  0x00
#define NODE_RESOLVING   0x01
#define NODE_RESOLVED    0x02
#define NODE_FAILED      0x03

/* Tree Hierarchy Levels */
#define TREE_ROOT       0x00    /* Level 0: Root */
#define TREE_TRUNK      0x01    /* Level 1: Core systems */
#define TREE_BRANCH     0x02    /* Level 2: Subsystems */
#define TREE_LEAF       0x03    /* Level 3: Services */

/* Compass Direction Spin States (π/4 increments) */
typedef enum {
    SPIN_NORTH      = 0,    /* 0° - π/4 */
    SPIN_NORTHEAST  = 1,    /* π/4 */
    SPIN_EAST       = 2,    /* π/2 */
    SPIN_SOUTHEAST  = 3,    /* 3π/4 */
    SPIN_SOUTH      = 4,    /* π */
    SPIN_SOUTHWEST  = 5,    /* 5π/4 */
    SPIN_WEST       = 6,    /* 3π/2 */
    SPIN_NORTHWEST  = 7     /* 7π/4 */
} SpinDirection;

/* Boot Sequence States */
typedef enum {
    STATE_SPARSE    = 0,    /* Inactive, half-spin allocated */
    STATE_REMEMBER  = 1,    /* Memory preservation */
    STATE_ACTIVE    = 2,    /* Full processing */
    STATE_VERIFY    = 3     /* NSIGII verification */
} BootState;

/* RIFT Header Structure (8 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t magic[4];       /* "NXOB" - OBINEXUS */
    uint8_t version;        /* 0x01 */
    uint8_t reserved;       /* 0x00 */
    uint8_t checksum;       /* XOR of header bytes = 0xFE */
    uint8_t flags;          /* Boot flags */
} RIFTHeader;

/* Quantum Qubit Representation */
typedef struct {
    uint8_t spin_direction; /* 0-7 compass direction */
    uint8_t half_spin;      /* π/4 unit rotations */
    uint8_t state;          /* SPARSE/ACTIVE/REMEMBER */
    uint8_t reserved;       /* Padding */
} Qubit;

/* Interdependency Node (Tree Hierarchy) */
typedef struct InterdepNode {
    uint8_t id;                     /* Node identifier */
    uint8_t level;                  /* TREE_ROOT/BRANCH/LEAF */
    uint8_t state;                  /* UNRESOLVED/RESOLVING/RESOLVED */
    uint8_t dependency_count;       /* Number of dependencies */
    struct InterdepNode **dependencies; /* Array of dependent nodes */
    void (*resolve_func)(struct InterdepNode *); /* Resolution function */
    void *data;                     /* Node-specific data */
} InterdepNode;

/* Ring Boot State Machine */
typedef struct {
    BootState current_state;
    BootState previous_state;
    uint8_t transition_count;
    uint8_t verification_code;      /* NSIGII_YES/NO/MAYBE */
    uint16_t flags;
} RingBootMachine;

/* Boot Sector Layout (512 bytes) */
typedef struct __attribute__((packed)) {
    RIFTHeader rift;                    /* 8 bytes */
    uint8_t    boot_code[502];          /* Executable code + data */
    uint8_t    signature[2];            /* 0x55 0xAA */
} BootSector;

/* Interdependency Tree */
typedef struct {
    InterdepNode *root;                 /* Root node */
    uint8_t node_count;                 /* Total nodes */
    uint8_t resolved_count;             /* Resolved nodes */
    uint8_t max_depth;                  /* Tree depth */
} InterdepTree;

/* Function Prototypes */
/* Interdependency System */
InterdepTree* interdep_tree_create(void);
void interdep_tree_destroy(InterdepTree *tree);
InterdepNode* interdep_node_create(uint8_t id, uint8_t level);
void interdep_add_dependency(InterdepNode *node, InterdepNode *dep);
int interdep_resolve_tree(InterdepTree *tree);
int interdep_resolve_node(InterdepNode *node);

/* Boot Sequence */
void mmuko_boot_init(void);
void mmuko_boot_sequence(void);
uint8_t nsigii_verify(RingBootMachine *machine);
void transition_state(RingBootMachine *machine, BootState new_state);
void half_spin_allocate(Qubit *q, SpinDirection dir);

/* Tree Hierarchy Boot */
void tree_boot_execute(InterdepTree *tree);
void tree_phase_sparse(InterdepTree *tree);
void tree_phase_remember(InterdepTree *tree);
void tree_phase_active(InterdepTree *tree);
void tree_phase_verify(InterdepTree *tree);

/* Utility */
void print_boot_message(const char *msg);
void halt_with_code(uint8_t code);
uint8_t calculate_checksum(uint8_t *data, size_t len);

#endif /* MMUKO_TYPES_H */
