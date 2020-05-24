#include <memory.h>
#include <dirent.h>
#include <stdint.h>
#include "db.h"
#include "util.h"

/*
 * Internal AVL tree used to store level data
 */

struct node {
    struct node *parent, *left, *right;
    int balance;

    struct level level;
};

// Allocate and initialize a node with parent and level data
static void node_create(struct node **node, struct node *parent, struct level *level) {
    *node = malloc(sizeof(struct node));
    (*node)->parent = parent;
    (*node)->left = (*node)->right = NULL;
    (*node)->balance = 0;
    (*node)->level = *level;
}

// Destruct and free a level and its field
static void node_destroy(struct node **node) {
    if ((*node)->level.field) {
        free((*node)->level.field);
    }

    free(*node);
}

// Initialize a tree
static void tree_create(struct node **root) {
    *root = NULL;
}

// Destruct a tree and destroy all of its nodes
static void tree_destroy(struct node **root) {
    if ((*root)->left) {
        tree_destroy(&(*root)->left);
    }
    if ((*root)->right) {
        tree_destroy(&(*root)->right);
    }
    node_destroy(root);
}

// Rotate the tree left
static void tree_rotate_l(struct node **root, struct node *node) {
    struct node *right = node->right;
    struct node *right_left = right->left;
    struct node *parent = node->parent;

    right->parent = parent;
    right->left = node;
    node->right = right_left;
    node->parent = right;

    if (right_left) {
        right_left->parent = node;
    }

    if (*root == node) {
        *root = right;
    } else if (parent->right == node) {
        parent->right = right;
    } else {
        parent->left = right;
    }

    right->balance--;
    node->balance = -right->balance;
}

// Rotate the tree right
static void tree_rotate_r(struct node **root, struct node *node) {
    struct node *parent = node->parent;
    struct node *left = node->left;
    struct node *left_right = left->right;

    left->parent = parent;
    left->right = node;
    node->left = left_right;
    node->parent = left;

    if (left_right) {
        left_right->parent = node;
    }

    if (*root == node) {
        *root = left;
    } else if (parent->left == node) {
        parent->left = left;
    } else {
        parent->right = left;
    }

    left->balance++;
    node->balance = -left->balance;
}

// Rotate the tree left-right
static void tree_rotate_lr(struct node **root, struct node *node) {
    struct node *parent = node->parent;
    struct node *left = node->left;
    struct node *left_right = left->right;
    struct node *left_right_right = left_right->right;
    struct node *left_right_left = left_right->left;

    left_right->parent = parent;
    node->left = left_right_right;
    left->right = left_right_left;
    left_right->left = left;
    left_right->right = node;
    left->parent = left_right;
    node->parent = left_right;

    if (left_right_right) {
        left_right_right->parent = node;
    }

    if (left_right_left) {
        left_right_left->parent = left;
    }

    if (*root == node) {
        *root = left_right;
    } else if (parent->left == node) {
        parent->left = left_right;
    } else {
        parent->right = left_right;
    }

    if (left_right->balance == 1) {
        node->balance = 0;
        left->balance = -1;
    } else if (left_right->balance == 0) {
        node->balance = 0;
        left->balance = 0;
    } else {
        node->balance = 1;
        left->balance = 0;
    }

    left_right->balance = 0;
}

// Rotate the tree right-left
static void tree_rotate_rl(struct node **root, struct node *node) {
    struct node *right = node->right;
    struct node *right_left = right->left;
    struct node *parent = node->parent;
    struct node *right_left_left = right_left->left;
    struct node *right_left_right = right_left->right;

    right_left->parent = parent;
    node->right = right_left_left;
    right->left = right_left_right;
    right_left->right = right;
    right_left->left = node;
    right->parent = right_left;
    node->parent = right_left;

    if (right_left_left) {
        right_left_left->parent = node;
    }

    if (right_left_right) {
        right_left_right->parent = right;
    }

    if (*root == node) {
        *root = right_left;
    } else if (parent->right == node) {
        parent->right = right_left;
    } else {
        parent->left = right_left;
    }

    if (right_left->balance == -1) {
        node->balance = 0;
        right->balance = 1;
    } else if (right_left->balance == 0) {
        node->balance = 0;
        right->balance = 0;
    } else {
        node->balance = -1;
        right->balance = 0;
    }

    right_left->balance = 0;
}

// Balance the tree
static void tree_balance(struct node **root, struct node *node, int balance) {
    while (node) {
        balance = (node->balance += balance);

        if (balance == 0) {
            return;
        } else if (balance == -2) {
            if (node->left->balance == -1) {
                tree_rotate_r(root, node);
            } else {
                tree_rotate_lr(root, node);
            }
            return;
        } else if (balance == 2) {
            if (node->right->balance == 1) {
                tree_rotate_l(root, node);
            } else {
                tree_rotate_rl(root, node);
            }
            return;
        }

        struct node *parent = node->parent;
        if (parent) {
            balance = (parent->left == node ? -1 : 1);
        }
        node = parent;
    }
}

// Insert a node with level data
static void tree_insert(struct node **root, struct level *level) {
    if (!*root) {
        node_create(root, NULL, level);
    } else {
        struct node *node = *root;
        while (node) {
            if (level->metadata.id < node->level.metadata.id) {
                struct node *left = node->left;
                if (!left) {
                    node_create(&node->left, node, level);
                    tree_balance(root, node, -1);
                    return;
                } else {
                    node = left;
                }
            } else if (level->metadata.id > node->level.metadata.id) {
                struct node *right = node->right;
                if (!right) {
                    node_create(&node->right, node, level);
                    tree_balance(root, node, 1);
                    return;
                } else {
                    node = right;
                }
            } else {
                node->level = *level;
                return;
            }
        }
    }
}

// Find a node by its level id
static bool tree_search(struct node **root, uint32_t id, struct node **out) {
    struct node *node = *root;
    while (node) {
        if (id < node->level.metadata.id) {
            node = node->left;
        } else if (id > node->level.metadata.id) {
            node = node->right;
        } else {
            *out = node;
            return true;
        }
    }

    return false;
}

// Get the first node in the tree
static bool tree_first(struct node **root, struct node **out) {
    struct node *node = *root;
    while (node && node->left) {
        node = node->left;
    }
    *out = node;
    return *root != NULL;
}

// Get the last node in the tree
static bool tree_last(struct node **root, struct node **out) {
    struct node *node = *root;
    while (node && node->right) {
        node = node->right;
    }
    *out = node;
    return *root != NULL;
}

// Get the next node in the tree, starting from the first when given NULL
static bool tree_next(struct node **root, struct node **out) {
    if (!*out) {
        return tree_first(root, out);
    }

    if ((*out)->right) {
        *out = (*out)->right;
        while ((*out)->left) {
            *out = (*out)->left;
        }
    } else {
        struct node *temp = *out;
        *out = (*out)->parent;
        while (*out && (*out)->right == temp) {
            temp = *out;
            *out = (*out)->parent;
        }
    }

    return *out != NULL;
}

/*
 * Metadata format manipulation
 */

// Incremental version correlated to layout of header
#define METADATA_FORMAT_VERSION 0

// Metadata file header
struct __attribute__((__packed__)) header {
    // "SSB"
    uint8_t magic[3];
    // File format version number
    uint8_t version;
    // Number of levels stored
    uint32_t num_levels;
};

// Load metadata entries from a file into the tree
static bool load_metadata(struct db *db) {
    char metadata_path[1024];
    strcpy(metadata_path, db->path);
    strcat(metadata_path, "/");
    strcat(metadata_path, "metadata.dat");

    // Open existing metadata file or create it
    FILE *metadata_file = fopen(metadata_path, "rb+");
    if (!metadata_file) {
        return false;
    }

    // Check file size
    fseek(metadata_file, 0, SEEK_END);
    long size = ftell(metadata_file);
    fseek(metadata_file, 0, SEEK_SET);
    if (size == 0) {
        return false;
    }

    // Parse the file header
    struct header header;
    if (!fread(&header, sizeof(struct header), 1, metadata_file)) {
        fprintf(stderr, "failed to parse metadata file header\n"
                        "aborting to prevent further corruption\n");
        exit(EXIT_FAILURE);
    }

    // Validate the magic
    if (header.magic[0] != 'S' || header.magic[1] != 'S' || header.magic[2] != 'B') {
        fprintf(stderr, "metadata file is not valid\n");
        exit(EXIT_FAILURE);
    }

    // Validate the format version
    if (header.version != METADATA_FORMAT_VERSION) {
        fprintf(stderr, "metadata file uses a different version (%d, expected %d)\n",
                header.version, METADATA_FORMAT_VERSION);
        exit(EXIT_FAILURE);
    }

    // Parse the metadata entries
    for (int i = 0; i < header.num_levels; i++) {
        struct metadata metadata;
        if (!fread(&metadata, sizeof(struct metadata), 1, metadata_file)) {
            fprintf(stderr, "failed to read metadata from entry %d\n", i);
            return false;
        }

        struct level level = {.metadata=metadata, .field=NULL};
        tree_insert(&db->tree, &level);
        db->num_levels++;
    }

    return true;
}

// Store metadata entries from the tree into a file
static bool store_metadata(struct db *db) {
    char metadata_path[1024];
    strcpy(metadata_path, db->path);
    strcat(metadata_path, "/");
    strcat(metadata_path, "metadata.dat");

    // Open metadata file truncated
    FILE *metadata_file = fopen(metadata_path, "w");
    if (!metadata_file) {
        fprintf(stderr, "failed to open metadata file for storing\n");
        return false;
    }

    // Write header
    struct header header = {
            .magic={'S', 'S', 'B'}, .version=METADATA_FORMAT_VERSION,
            .num_levels=db->num_levels
    };
    if (!fwrite(&header, sizeof(struct header), 1, metadata_file)) {
        fprintf(stderr, "failed to write header to metadata file\n");
        return false;
    }

    // Write metadata entries
    struct node *node = NULL;
    while (tree_next(&db->tree, &node)) {
        if (!fwrite(&node->level.metadata, sizeof(struct metadata), 1, metadata_file)) {
            fprintf(stderr, "failed to write metadata entry\n");
            return false;
        }
    }

    fclose(metadata_file);
    return true;
}

// Load a level's field from a file
static bool load_level(struct db *db, struct level *level) {
    char field_path[1024];
    sprintf(field_path, "%s/%d.txt", db->path, level->metadata.id);

    FILE *field_file = fopen(field_path, "rb+");
    if (!field_file) {
        fprintf(stderr, "failed to load field file for level %d\n",
                level->metadata.id);
    }

    // For convenience, we store levels in a text editor-friendly manner:
    // encoded in UTF-8 with proper line endings. This is where we pay the tax

    char buffer[8096] = {0};
    fread(buffer, sizeof(char), 8096, field_file);

    level->field = calloc(80 * 25, sizeof(uint32_t));

    char *p = buffer;
    uint32_t *field = level->field;
    int num_columns = 0, num_rows = 0;
    while (*p) {
        uint32_t value = utf8_decode(&p);

        // Ignore carriage returns
        if (value == 0x0d) continue;
        else if (value == 0x0a) {
            if (num_columns != 80) {
                goto failure;
            }

            num_rows++;
            num_columns = 0;
        }
        else {
            *field++ = value;
            num_columns++;
        }
    }

    if (!(num_rows == 25 && num_columns == 0) && !(num_rows == 24 && num_columns == 80)) {
        goto failure;
    }

    return true;

    failure:
    free(level->field);
    level->field = NULL;

    return false;
}

/*
 * Level database interface
 */

bool db_create(struct db *db, char *path) {
    db->path = path;
    db->num_levels = 0;

    tree_create(&db->tree);

    // Load the metadata
    if (!load_metadata(db)) {
        fprintf(stderr, "no previous metadata found: creating file\n");
    }

    // Handle inconsistencies between metadata and fields
    // If a level is on disk but not in the metadata: add it
    // If a level is in the metadata but not on disk: fail hard

    // Loop over all of the fields without caring about order
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "failed to open level directory \"%s\"\n", path);
        return false;
    }

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        char *dot = strrchr(ent->d_name, '.');
        if (!dot || dot == ent->d_name || strcmp(dot + 1, "txt") != 0) {
            continue;
        }

        char *end;
        uint32_t id = (uint32_t) strtol(ent->d_name, &end, 10);
        if (end != dot) {
            continue;
        }

        // Add the level if not present
        struct node *node = NULL;
        if (!tree_search(&db->tree, id, &node)) {
            struct level level = {0};
            level.metadata.id = id;
            tree_insert(&db->tree, &level);
            db->num_levels++;
        }

    }

    closedir(dir);

    return true;
}

void db_destroy(struct db *db) {
    if (!store_metadata(db)) {
        fprintf(stderr, "unable to store metadata to file\n");
    }

    tree_destroy(&db->tree);
}

bool db_get_metadata(struct db *db, uint32_t id, struct metadata **metadata) {
    struct node *node = NULL;
    if (!tree_search(&db->tree, id, &node) || !node) {
        return false;
    }

    *metadata = &node->level.metadata;
    return true;
}

bool db_get_level(struct db *db, uint32_t id, struct level **level) {
    struct node *node = NULL;
    if (!tree_search(&db->tree, id, &node) || !node) {
        return false;
    }

    // Load the field if it hasn't been accessed yet
    if (!node->level.field) {
        if (!load_level(db, &node->level) || !node->level.field) {
            fprintf(stderr, "failed to load level %d\n", id);
            return false;
        }
    }

    *level = &node->level;
    return true;
}

bool db_create_level(struct db *db, char *name, uint32_t *field, struct level **out) {
    uint32_t last_id = 0;

    struct node *last_node = NULL;
    if (tree_last(&db->tree, &last_node)) {
        last_id = last_node->level.metadata.id;
    }

    struct level level = {0};
    level.metadata.id = last_id + 1;
    (void)level.metadata.name; //TODO
    level.field = malloc(80 * 25 * sizeof(uint32_t));
    memcpy(level.field, field, 80 * 25 * sizeof(uint32_t));

    return true;
}
