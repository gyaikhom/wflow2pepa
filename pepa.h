/*********************************************************************

  THE ENHANCE PROJECT
  School of Informatics,
  University of Edinburgh,
  Edinburgh - EH9 3JZ
  United Kingdom


  DESCRIPTION:

  This file contains all the function prototypes and data structures
  that are used by the parser and the lexical analyser.

  Written by: Gagarine Yaikhom

*********************************************************************/

#ifndef __PEPA_SKELTREE_H
#define __PEPA_SKELTREE_H

#define MAX_SKEL_NAME 24   /* Max. length of skeleton names. */
#define MAX_NSOURCES 50    /* Max. length of source list. */
#define MAX_NSINKS 50      /* Max. length of sink list. */
#define SHOW_INDEX 0x0000  /* Flag which shows node index. */
#define SHOW_PRED  0x0001  /* Flag which shows predecessor skeleton. */
#define SHOW_SUCC  0x0002  /* Flag which shows successor skeleton. */

/* We assume that system being model is a part of a bigger
   system. Therefore, while building the workflow system from the
   skeleton hierarchy tree, we allow the boundary tasks to communicate
   with the external system through these dummy interfaces. */
#define SOURCE_MEM -1
#define SINK_MEM -1

/* Type of skeleton hierarchy tree component. */
typedef enum {
    UNKNOWN = 0,  /* Interface with outside. */
    PIPE,         /* Pipeline (sequential). */
    DEAL,         /* Round-robin distribution. */
    FARM,         /* Farm (non-deterministic) */
    TASK,         /* Task function (leaf-node) */
    NSKEL         /* Number of patterns supported. */
} __htree_comp_t; /* Hierarchy tree component. */

typedef struct __htree_node_s __htree_node_t;
typedef struct __htree_child_s {
    struct __htree_child_s *prev; /* Pointer to the previous sibling. */
    struct __htree_child_s *next; /* Pointer to the next sibling. */
    __htree_node_t *child;        /* Children list pointer. */
} __htree_child_t;                /* Child of non-leaf component. */

typedef struct __htree_plist_s {
    int n;         /* Number of indices in list. */
    int *l;        /* Index list pointer. */
} __htree_plist_t; /* Source/destination index list. */

/* The child list is required for the current implementation because
  a node can have more than two children. In future versions, we can
  transform this tree into a Binary tree by imposing the restriction
  that a node can only have two children. This, I think, should not be   
  a big deal because we can always group all the remaining siblings,
  except the first sibling into a single node. */
struct __htree_node_s {
    char name[256];         /* Name of the task. */
    double rate;            /* Task rate. */
    int nchr;               /* Number of children required. */
    int nchx;               /* Number of children created. */
    int index;              /* Node index in the hierarchy tree. */
    int rank;               /* My sibling rank. */
    __htree_node_t *p;      /* Pointer to parent node. */
    __htree_child_t *clist; /* Children list. */
    __htree_comp_t mtype;   /* Skeleton type of this node. */
    __htree_comp_t ptype;   /* Predecessor skeleton type. */
    __htree_comp_t stype;   /* Successor skeleton type. */
    __htree_plist_t sol;    /* Source index list. */
    __htree_plist_t sil;    /* Sink index list */
    __htree_plist_t temp;   /* Used for Deals and Farms. */
};

/* Currently, we only support one skeleton hierarchy tree. Newer
   versions should support multiple skeleton hierarchy trees by
   maintating instances of the hierarchy tree data structure in the
   following runtime system data structure. */
struct __htree_rt_s {
    __htree_node_t *htree;     /* Skeleton hierarchy tree. */
    __htree_node_t *curr_node; /* Current node. */
    __htree_node_t **sstab;    /* Source-sink lookup table. */
    int nnodes;                /* Number of nodes in the tree. */
    int nleaves;               /* Number of leaf nodes. */
    int node_sum;              /* Used to validate tree structure. */
    char **hnames;             /* Hostnames of available processes. */
};                             /* Runtime system. */
extern struct __htree_rt_s __htree_rt;

/* Flags for output behaviour:
   1. If graph is set, .dot graphs are generated.
   2. If latex is set, a LaTeX file is generated.
   3. If output is set, the PEPA model is written into a file.
   4. If complete is set, everything is done (takes longer due to
   invocation of external applications such as dot, pdflatex
   etc.). */
extern int graph, latex, output, complete;
extern char *fname;

/* This function displays the stucture of the skeleton hierarchy
   tree. The flag determines which informations are displayed in the
   structure (see above definition with SHOW_ prefix). */
extern int htree_display(unsigned short flags);

/* Once the description files have been read by the parser, it
   generates the source-sink lookup table from the skeleton hierarcy
   tree. This function finalises the skeleton hierarchy tree, after
   which the structure of the tree cannot be changed. */
extern int htree_commit(void);

/* This function is used to display the source-sink lookup table.
   It is very useful while debugging. */
extern int __htree_display_sstab(void);

/* Insert a subtree node to the parent node. If the skeleton type if
   TASK then we are required to have the function pointer to the
   stage function as the variable argument. */
extern int __htree_insert_node(__htree_comp_t skel, int nchild, ...);

/* The function generates the source-sink lookup table.
   This table  contains, for every node, the source list,
   the sink list, and other relevant information that are
   derived from the skeleton hierarchy tree. */
extern int __htree_generate_sstab(__htree_node_t *n, int source, int sink);

/* Destroys the skeleton hierarchy tree by deallocating memory used by
   the tree data structures.*/
extern void __htree_destroy(__htree_node_t *n);

/* The following macros are used to insert nodes based on the
   different patterns. As we can see, we have a generic function which
   can be used to insert any type of node into the skeleton hierarchy
   tree. These macros specialises this function to insert the
   appropriate node type. */
#define pipe(X) __htree_insert_node(PIPE, (X))
#define deal(X,Y,R)                             \
    {                                           \
        short i;                                \
        __htree_insert_node(DEAL, (X));         \
        for (i = 0; i < (X); i++)               \
            __htree_insert_node(TASK, 0, Y, R); \
    }
#define xdeal(X) __htree_insert_node(DEAL, (X))
#define farm(X,Y,R)                             \
    {                                           \
        short i;                                \
        __htree_insert_node(FARM, (X));         \
        for (i = 0; i < (X); i++)               \
            __htree_insert_node(TASK, 0, Y, R); \
    }
#define xfarm(X) __htree_insert_node(FARM, (X))
#define task(X,R) __htree_insert_node(TASK, 0, X, R)

#endif /* __PEPA_SKELTREE_H */
