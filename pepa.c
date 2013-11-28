/*********************************************************************

  THE ENHANCE PROJECT
  School of Informatics,
  University of Edinburgh,
  Edinburgh - EH9 3JZ
  United Kingdom


  DESCRIPTION:

  This file contains all the functions that are called by the parser
  to build a skeleton hierarchy tree from a hierarchical description
  file. There are also functions which is used to generate the
  corresponding Performance Evaluation Process Algebra (PEPA) based
  performance models from the skeleton hierarchy tree.

  Written by: Gagarine Yaikhom

*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "pepa.h"

/* This list all the skeleton or pattern names that are currently
   supported in the description file. Please check the lexical
   analyser for more details. */
char skel_name[NSKEL][MAX_SKEL_NAME] = {
    "task",   /* Task (leaf-node). */
    "pipe",   /* Pipeline skeleton. */
    "deal",   /* Deal skeleton. */
    "farm",   /* Farm skeleton. */
    "unknown" /* Unkown skeleton. */
};

/* This is the runtime system which contains all the data structures
   that are relevant to the entire application. See "pepe.h" for its
   contents and explanations. */
struct __htree_rt_s __htree_rt;

/* These are the flags which affect the application behaviour. */
int graph = 0, latex = 0, output = 0, complete = 0;

/* File streams for input and output. */
char *fname = NULL;
FILE *output_file = NULL;

/* CRITICAL:
   The following data structures are fine for prototyping. However,
   the memory inefficieny is quite discouraging. This should be
   improved by using a buffering scheme. */
static char process[10240];   /* Process definitions. */
static char set[10240];       /* Synchronisation sets. */
static char model[10240];     /* System equation. */
static char temp[10240];      /* Temporary buffer. */
static char temp_sset[10240]; /* Temporary sync. set. */


/* Used to inherit source list from the parent node. */
static void __htree_inherit_source(__htree_node_t *n);

/* Used to update the source list on the parent node
   based on the current child subtree. */
static void __htree_update_source(__htree_node_t *n);

/* Recursive function for generating all the source lists
   for each of the nodes in the skeleton hierarchy tree. */
static void __htree_generate_source(__htree_node_t *n);

/* Used to inherit sink sink list from the parent node. */
static void __htree_inherit_sink(__htree_node_t *n);

/* Used to update the sink list on the parent node
   based on the current child subtree. */
static void __htree_update_sink(__htree_node_t *n);

/* Recursive function for generating all the source lists
   for each of the nodes in the skeleton hierarchy tree. */
static void __htree_generate_sink(__htree_node_t *n);

/* When a skeleton node contains a set of children nodes,
   this set is maintained as a double linked lists. This
   function is used to insert a new node into the existing
   child list of the current skeleton node. */
static int __htree_insert_sibling(__htree_node_t *n);


void __htree_generate_source(__htree_node_t *n) {
    __htree_child_t *c;
    short i;
    __htree_inherit_source(n);
    if (n->mtype == TASK) __htree_rt.sstab[n->index] = n;
    if ((c = n->clist))
        for (i = 0; c && i < n->nchr; i++) {
            if (c->child)
                __htree_generate_source (c->child);
            c = c->next;
        }
    __htree_update_source (n);
}

void __htree_inherit_source(__htree_node_t *n) {
    short i;
    n->sol.n = n->p->sol.n;
    n->sol.l = (int *) malloc(sizeof(int)*n->sol.n);
    n->ptype = n->p->ptype;
    for (i = 0; i < n->sol.n; i++)
        n->sol.l[i] = n->p->sol.l[i];
}

void __htree_update_source(__htree_node_t *n) {
    __htree_child_t *c;
    short i, j;

    n->temp.l = NULL;    
    switch (n->mtype) {
    case TASK:
        if ((n->p->mtype != DEAL) &&
            (n->p->mtype != FARM)) {
            n->p->sol.n = 1;
            n->p->sol.l[0] = n->index;
            n->p->ptype = PIPE;
        } else {
            n->temp.n = 1;
            n->temp.l = (int *) malloc(sizeof(int));
            n->temp.l[0] = n->index;
        } 
        break;
        
    case PIPE:
        if ((n->p->mtype != DEAL) &&
            (n->p->mtype != FARM)) {
            free (n->p->sol.l);
            n->p->sol.n = n->sol.n;
            n->p->sol.l = (int *) malloc(sizeof(int)*(n->p->sol.n));
            for (i = 0; i < n->p->sol.n; i++)
                n->p->sol.l[i] = n->sol.l[i];
            n->p->ptype = n->ptype;
        } else {
            n->temp.n = n->sol.n;
            n->temp.l = (int *) malloc(sizeof(int)*(n->temp.n));
            for (i = 0; i < n->temp.n; i++)
                n->temp.l[i] = n->sol.l[i];
        }
        break;

    case DEAL:
    case FARM:
        if ((n->p->mtype != DEAL) &&
            (n->p->mtype != FARM)) {
            free (n->p->sol.l);
            n->p->sol.n = 0;
            n->p->sol.l = (int *) malloc(sizeof(int)*MAX_NSOURCES);
            c = n->clist;
            for (i = 0; i < n->nchr; i++) {
                for (j = 0; j < c->child->temp.n; j++) {
                    n->p->sol.l[n->p->sol.n] = c->child->temp.l[j];
                    n->p->sol.n++;
                }
                free(c->child->temp.l);
                c = c->next;
            }
            realloc (n->p->sol.l, sizeof(int)*n->p->sol.n);
            n->p->ptype = n->mtype;
        } else {
            n->temp.n = 0;
            n->temp.l = (int *) malloc(sizeof(int)*MAX_NSOURCES);
            c = n->clist;
            for (i = 0; i < n->nchr; i++) {
                for (j = 0; j < c->child->temp.n; j++) {
                    n->temp.l[n->temp.n] = c->child->temp.l[j];
                    n->temp.n++;
                }
                free(c->child->temp.l);
                c = c->next;
            }
            realloc(n->temp.l, sizeof(int)*n->temp.n);
        }
        break;

    case UNKNOWN:
    case NSKEL:
        break;
    }
}

void __htree_generate_sink(__htree_node_t *n) {
    __htree_child_t *c;
    short i;

    __htree_inherit_sink (n);
    if (n->mtype == TASK) __htree_rt.sstab[n->index] = n;
    if (n->clist) {
        c = n->clist->prev;
        for (i = 0; c && i < n->nchr; i++) {
            if (c->child)
                __htree_generate_sink(c->child);
            c = c->prev;
        }
    }
    __htree_update_sink (n);
}

void __htree_inherit_sink(__htree_node_t *n) {
    short i;
    n->sil.n = n->p->sil.n;
    n->sil.l = (int *) malloc(sizeof(int)*n->sil.n);
    n->stype = n->p->stype;
    for (i = 0; i < n->sil.n; i++)
        n->sil.l[i] = n->p->sil.l[i];
}

void __htree_update_sink(__htree_node_t *n) {
    __htree_child_t *c;
    short i, j;

    switch (n->mtype) {
    case TASK:
        if ((n->p->mtype != DEAL) &&
            (n->p->mtype != FARM)) {
            n->p->sil.n = 1;
            n->p->sil.l[0] = n->index;
            n->p->stype = PIPE;
        } else {
            n->temp.n = 1;
            n->temp.l = (int *) malloc(sizeof(int));
            n->temp.l[0] = n->index;
        }
        break;
        
    case PIPE:
        if ((n->p->mtype != DEAL) &&
            (n->p->mtype != FARM)) {
            free (n->p->sil.l);
            n->p->sil.n = n->sil.n;
            n->p->sil.l = (int *) malloc(sizeof(int)*(n->p->sil.n));
            for (i = 0; i < n->p->sil.n; i++)
                n->p->sil.l[i] = n->sil.l[i];
            n->p->stype = n->stype;
        } else {
            n->temp.n = n->sil.n;
            n->temp.l = (int *) malloc(sizeof(int)*(n->temp.n));
            for (i = 0; i < n->temp.n; i++)
                n->temp.l[i] = n->sil.l[i];
        }
        break;
        
    case DEAL:
    case FARM:
        if ((n->p->mtype != DEAL) &&
            (n->p->mtype != FARM)) {
            free (n->p->sil.l);
            n->p->sil.n = 0;
            n->p->sil.l = (int *) malloc(sizeof(int)*MAX_NSINKS);
            c = n->clist;
            for (i = 0; i < n->nchr; i++) {
                for (j = 0; j < c->child->temp.n; j++) {
                    n->p->sil.l[n->p->sil.n] = c->child->temp.l[j];
                    n->p->sil.n++;
                }
                c = c->next;
            }
            realloc (n->p->sil.l, sizeof(int)*n->p->sil.n);
            n->p->stype = n->mtype;
        } else {
            n->temp.n = 0;
            n->temp.l = (int *) malloc(sizeof(int)*MAX_NSINKS);
            c = n->clist;
            for (i = 0; i < n->nchr; i++) {
                for (j = 0; j < c->child->temp.n; j++) {
                    n->temp.l[n->temp.n] = c->child->temp.l[j];
                    n->temp.n++;
                }
                c = c->next;
            }
            realloc (n->temp.l, sizeof(int)*n->temp.n);
        }
        break;

    case UNKNOWN:
    case NSKEL:
        break;
    }
}


int __htree_insert_sibling (__htree_node_t *n) {
    __htree_child_t *m;

    /* Create a new sibling node for the double linke dist. */
    if (!(m = (__htree_child_t *) malloc(sizeof(__htree_child_t))))
        return -1;

    /* Each sibling node can have a subtree of its own. */
    m->child = n;

    /* What is my sibling rank in the double linked list? The head
       (the first sibling inserted has rank zero. */
    n->rank = __htree_rt.curr_node->nchx;
    if (__htree_rt.curr_node->clist) {
        /* If there are siblings in the double linked list, append self
           at the end of the list. Remeber to update the following
           pointer.
           o The new sibling should point to the head and the last.
           o The previous last should point to the new as follower.
           o The head of the double linked list (the first sibling)
           should point to the new sibling as previous. */
        m->prev = __htree_rt.curr_node->clist->prev;
        m->next = __htree_rt.curr_node->clist;
        __htree_rt.curr_node->clist->prev->next = m;
        __htree_rt.curr_node->clist->prev = m;
    } else {
        /* If this is the first child, this child self
           references until more siblings are inserted. */
        m->prev = m;
        m->next = m;
        __htree_rt.curr_node->clist = m;                
    }
    /* Acknowledge that a new sibling has entered. This is required to
       test if the parent node has created all the required children. */
    __htree_rt.curr_node->nchx++;
    __htree_rt.node_sum--;
    return 0;
}

/* Used to output the hierarchical structure of the
   skeleton hierarchy tree. */
void __htree_write_tree(FILE *f, __htree_node_t *n,
                        short ind, short l) {
    __htree_child_t *c;
    short i, j;

    if (n) {
        fprintf(f, "%%");
        for (i = 0; i < l; i++) {
            for (j = 0; j < 4; j++) fprintf(f, " ");
            fprintf(f, "|");
        }
        for (i = 0; i < 2; i++) fprintf(f, "_");
        if (n->mtype != TASK)
            fprintf(f, "%s", skel_name[n->mtype]);
        else
            fprintf(f, "%d", n->index);

        fprintf(f, "\n");
    }
    c = n->clist;
    for (i = 0; c && i < n->nchr; i++) {
        __htree_write_tree(f, c->child, ind + 4, l + 1);
        c = c->next;
    }
}
int htree_write_tree(FILE *f) {
    if (!__htree_rt.htree || __htree_rt.node_sum) {
        printf ("Invalid tree.\n");
        return -1;
    }
    fprintf(f, "%% PEPA model generated from skeleton-based\n"
            "%% hierarchical task arrangement.\n\n");
    __htree_write_tree(f, __htree_rt.htree, 4, 0);
    fprintf(f, "\n");
    return 0;
}

/* Used for displaying the source-sink lookup table. */
int __htree_display_sstab(void) {
    int i, j;
    printf("--------------------------\n"
           " TASK | Source |  Sink \n"
           "--------------------------\n");
    for (i = 0; i < __htree_rt.nleaves; i++) {
        printf("%3d\t", i);
        printf("[%d", __htree_rt.sstab[i]->sol.l[0]);
        for (j = 1; j < __htree_rt.sstab[i]->sol.n; j++)
            printf(" %d", __htree_rt.sstab[i]->sol.l[j]);
        printf("]\t[%d", __htree_rt.sstab[i]->sil.l[0]);
        for (j = 1; j < __htree_rt.sstab[i]->sil.n; j++)
            printf(" %d", __htree_rt.sstab[i]->sil.l[j]);
        printf("]\n");
    }
    printf ("--------------------------\n");
    return 0;
}

/* Used for generating the .dot graph representation. */
int htree_write_graph(void) {
    int i, j, k;
    FILE *f;
    char temp[64];

    strcpy(temp, fname);
    strcat(temp, "dot");
    f = fopen(temp, "w");
    fprintf(f,
            "digraph \"%sdes\" {\n"
            "graph [rankdir=LR, size=\"8,8\", splines=true];\n"
            "edge [arrowhead=open,color=\"#777777\"];\n"
            "node [height=0.5, shape = polygon, "
            "fontsize=16, color=\"#888888\", "
            "style=filled, fillcolor=\"#dddddd\"];\n", fname);
    for (i = 0; i < __htree_rt.nleaves - 1; i++)
        for (j = 0; j < __htree_rt.sstab[i]->sil.n; j++) {
            k =    __htree_rt.sstab[i]->sil.l[j];
            fprintf (f, "\"%s %d\" -> \"%s %d\"\n",
                     __htree_rt.sstab[i]->name, i,
                     __htree_rt.sstab[k]->name, k);
        }
    fprintf (f, "}");
    fclose(f);
    return 0;
}

/* This is the source-sink skeleton/pattern matrix.
   This matrix is used to determine the source and
   sink communication pattern combination which
   determines the performance model for the task. */ 
int pattern_matrix[4][4] = {
    /* -------------------------------------- */
    /*         | Unknown | Pipe | Deal | Farm */
    /* -------------------------------------- */
    /* Unknown |    0    |   1  |   2  |   3  */
    /*    Pipe |    4    |   5  |   6  |   7  */
    /*    Deal |    8    |   9  |  10  |  11  */
    /*    Farm |   12    |  13  |  14  |  15  */
    /* -------------------------------------- */
    { 0,  1,  2,  3},
    { 4,  5,  6,  7},
    { 8,  9, 10, 11},
    {12, 13, 14, 15}
};

/* Find the greatest common divisor using Euclid's algorithm. */
int gcd(int a, int b) {
    while (a != b) {
        if (a > b) a = a - b;
        else b = b - a;
    }
    return a;
}

/* Find the lowest common multiple. */
#define lcm(a,b) (((a)*(b))/gcd((a),(b)))

/* Generates the process definition for this leaf-node. */
int __htree_task_def(__htree_node_t *node) {
    int i, j, k, l;
    if (pattern_matrix[node->ptype][node->stype] == 0) {
        printf("Error\n");
        return -1;
    }
    if ((node->sol.n > 0) && (node->sil.n > 0))
        l = lcm(node->sol.n, node->sil.n);
    else
        l = node->sol.n + node->sil.n;
    if (latex) {
        sprintf (temp, "t_{%d} & \\rmdef & ", node->index);
        strcat(process, temp);
    }
    fprintf(output_file, "t_%d = \t", node->index);
    switch(pattern_matrix[node->ptype][node->stype]) {
    case 1:
    case 2:
        for (i = 0; i < l; i++) {
            k = node->sil.l[i % node->sil.n];
            if (latex) {
                sprintf(temp, "(comp_{%d}, %f).(move_{%d,%d}, \\infty).%s",
                        node->index, node->rate, node->index, k,
                        ((l > 1) && (i < l - 1)) ? "\\\\&&" : "");
                strcat(process, temp);
            }
            fprintf(output_file, "(comp_%d, %f).(move_%d_%d, infty).%s",
                    node->index, node->rate, node->index, k,
                    ((l > 1) && (i < l - 1)) ? "\n\t" : "");
        }
        break;
    case 4:
    case 8:
        for (i = 0; i < l; i++) {
            j = node->sol.l[i % node->sol.n];
            if (latex) {
                sprintf(temp, "(move_{%d,%d}, \\infty).(comp_{%d}, %f).%s",
                        j, node->index, node->index, node->rate,
                        ((l > 1) && (i < l - 1)) ? "\\\\&&" : "");
                strcat(process, temp);
            }
            fprintf(output_file, "(move_%d_%d, infty).(comp_%d, %f).%s",
                    j, node->index, node->index, node->rate,
                    ((l > 1) && (i < l - 1)) ? "\n\t" : "");
        }
        break;        
    case 5:
    case 6:
    case 9:
    case 10:
        for (i = 0; i < l; i++) {
            k = node->sil.l[i % node->sil.n];
            j = node->sol.l[i % node->sol.n];
            if (latex) {
                sprintf(temp, "(move_{%d,%d}, \\infty).(comp_{%d}, %f)."
                        "(move_{%d,%d}, \\infty).%s",
                        j, node->index, node->index, node->rate, node->index, k,
                        ((l > 1) && (i < l - 1)) ? "\\\\&&" : "");
                strcat(process, temp);
            } 
            fprintf(output_file, "(move_%d_%d, infty).(comp_%d, %f)."
                    "(move_%d_%d, infty).%s",
                    j, node->index, node->index, node->rate, node->index, k,
                    ((l > 1) && (i < l - 1)) ? "\n\t" : "");
        }
        break;        
    case 3:
        if (latex) {
            sprintf(temp, "(comp_{%d}, %f).t_{%d}^{0};\\\\"
                    "t_{%d}^{0} & \\rmdef & "
                    "(move_{%d,%d}, \\infty).t_{%d}",
                    node->index, node->rate, node->index, node->index,
                    node->index, node->sil.l[0], node->index);
            strcat(process, temp);
            for (i = 1; i < node->sil.n; i++) {
                sprintf(temp, "\\\\&&+ (move_{%d,%d}, \\infty).",
                        node->index, node->sil.l[i % node->sil.n]);
                strcat(process, temp);
                if ((node->sil.n > 1) && (i < node->sil.n - 1)) {
                    sprintf(temp, "t_{%d}", node->index);
                    strcat(process, temp);
                }
            }
        }
        fprintf(output_file, "(comp_%d, %f).t_%d_0;\nt_%d_0 = "
                "(move_%d_%d, infty).t_%d",
                node->index, node->rate, node->index, node->index,
                node->index, node->sil.l[0], node->index);
        for (i = 1; i < node->sil.n; i++) {
            fprintf(output_file, "\n\t+ (move_%d_%d, infty).",
                    node->index, node->sil.l[i % node->sil.n]);
            if ((node->sil.n > 1) && (i < node->sil.n - 1))
                fprintf(output_file, "t_%d", node->index);
        }
        break;
    case 12:
        if (latex) {
            sprintf(temp, "(move_{%d,%d}, \\infty).t_{%d}^{0}",
                    node->sol.l[0], node->index, node->index);
            strcat(process, temp);
            for (i = 1; i < node->sol.n; i++) {
                sprintf(temp, "\\\\&&+ (move_{%d,%d}, \\infty).t_{%d}^{0}",
                        node->sol.l[i], node->index, node->index);
                strcat(process, temp);
            }
            sprintf(temp, ";\\\\t_{%d}^{0} & \\rmdef & (comp_{%d}, %f).",
                    node->index, node->index, node->rate);
            strcat(process, temp);
        }
        fprintf(output_file, "(move_%d_%d, infty).t_%d_0",
                node->sol.l[0], node->index, node->index);
        for (i = 1; i < node->sol.n; i++) {
            fprintf(output_file, "\n\t+ (move_%d_%d, infty).t_%d_0",
                    node->sol.l[i], node->index, node->index);
        }
        fprintf(output_file, ";\nt_%d_0 = (comp_%d, %f).",
                node->index, node->index, node->rate);
        break;
    case 7:
    case 11:
        if (latex) {
            for (i = 0; i < node->sol.n; i++) {
                sprintf(temp,
                        "(move_{%d,%d}, \\infty).(comp_{%d}, %f).t_{%d}^{%d};\\\\"
                        "t_{%d}^{%d} & \\rmdef & (move_{%d,%d}, \\infty).",
                        node->sil.l[i], node->index, node->index, node->rate,
                        node->index, i, node->index, i, node->index, node->sil.l[0]);
                strcat(process, temp);
                if (i < node->sol.n - 1)
                    sprintf(temp, "t_{%d}^{%d}",    node->index, i+1);
                else
                    sprintf(temp, "t_{%d}", node->index);
                strcat(process, temp);
                for (j = 1; j < node->sil.n; j++) {
                    sprintf(temp,
                            "\\\\&&+ (move_{%d,%d}, \\infty).",
                            node->index, node->sil.l[j]);
                    strcat(process, temp);
                    if (i < node->sol.n - 1) {
                        sprintf(temp, "t_{%d}^{%d}", node->index, i+1);
                        strcat(process, temp);
                    } else {
                        if (j < node->sil.n - 1) {
                            sprintf(temp, "t_{%d}", node->index);
                            strcat(process, temp);
                        }
                    }
                }
            }
        }
        for (i = 0; i < node->sol.n; i++) {
            fprintf(output_file,
                    "(move_%d_%d, infty).(comp_%d, %f).t_%d_%d;\n"
                    "t_%d_%d = (move_%d_%d, infty).",
                    node->sil.l[i], node->index, node->index, node->rate,
                    node->index, i, node->index, i, node->index, node->sil.l[0]);
            if (i < node->sol.n - 1)
                fprintf(output_file, "t_%d_%d",    node->index, i+1);
            else
                fprintf(output_file, "t_%d", node->index);
            for (j = 1; j < node->sil.n; j++) {
                fprintf(output_file,
                        "\n\t+ (move_%d_%d, infty).",
                        node->index, node->sil.l[j]);
                if (i < node->sol.n - 1)
                    fprintf(output_file, "t_%d_%d",    node->index, i+1);
                else {
                    if (j < node->sil.n - 1)
                        fprintf(output_file, "t_%d", node->index);
                }
            }
        }
        break;
    case 13:
    case 14:
        if (latex) {
            for (i = 0, k = 0; i < node->sil.n; i++) {
                sprintf(temp,
                        "(move_{%d,%d}, \\infty).t_{%d}^{%d}",
                        node->sol.l[0], node->index, node->index, k);
                strcat(process, temp);
                for (j = 1; j < node->sol.n; j++) {
                    sprintf(temp,
                            "\\\\&&+ (move_{%d,%d}, \\infty).t_{%d}^{%d}",
                            node->sol.l[j], node->index, node->index, k);
                    strcat(process, temp);
                }
                sprintf(temp,
                        ";\\\\t_{%d}^{%d} & \\rmdef & "
                        "(comp_{%d}, %f).(move_{%d,%d}, \\infty).",
                        node->index, k, node->index, node->rate,
                        node->index, node->sil.l[i]);
                strcat(process, temp);
                if (i < node->sil.n - 1) {
                    sprintf(temp, "t_{%d}^{%d};\\\\"
                            "t_{%d}^{%d} & \\rmdef & ",
                            node->index, k+1, node->index, k+1);
                    strcat(process, temp);
                }
                k += 2;
            }
        }
        for (i = 0, k = 0; i < node->sil.n; i++) {
            fprintf(output_file,
                    "(move_%d_%d, infty).t_%d_%d",
                    node->sol.l[0], node->index, node->index, k);
            for (j = 1; j < node->sol.n; j++) {
                fprintf(output_file,
                        "\n\t+ (move_%d_%d, infty).t_%d_%d",
                        node->sol.l[j], node->index, node->index, k);
            }
            fprintf(output_file,
                    ";\nt_%d_%d = (comp_%d, %f).(move_%d_%d, infty).",
                    node->index, k, node->index, node->rate,
                    node->index, node->sil.l[i]);
            if (i < node->sil.n - 1) {
                fprintf(output_file, "t_%d_%d;\nt_%d_%d = ",
                        node->index, k+1, node->index, k+1);
            }
            k += 2;
        }
        break;
    case 15:
        if (latex) {
            sprintf(temp, 
                    "(move_{%d,%d}, \\infty).(comp_{%d}, %f).t_{%d}^{0}",
                    node->sol.l[0], node->index, node->index,
                    node->rate, node->index);
            strcat(process, temp);
            for (i = 1; i < node->sol.n; i++) {
                sprintf(temp, 
                        "\\\\&&+ (move_{%d,%d}, \\infty).(comp_{%d}, %f).t_{%d}^{0}",
                        node->sol.l[i], node->index, node->index,
                        node->rate, node->index);
                strcat(process, temp);
            }
            sprintf(temp, 
                    ";\\\\t_{%d}^{0} & \\rmdef & (move_{%d,%d}, \\infty).",
                    node->index, node->index, node->sil.l[0]);
            strcat(process, temp);
            for (i = 1; i < node->sil.n; i++) {
                sprintf(temp, 
                        "t_{%d}\\\\&&+ (move_{%d,%d}, \\infty).",
                        node->index, node->index, node->sil.l[i]);
                strcat(process, temp);
            }
        }
        fprintf(output_file,
                "(move_%d_%d, infty).(comp_%d, %f).t_%d_0",
                node->sol.l[0], node->index, node->index,
                node->rate, node->index);
        for (i = 1; i < node->sol.n; i++) {
            fprintf(output_file,
                    "\n\t+ (move_%d_%d, infty).(comp_%d, %f).t_%d_0",
                    node->sol.l[i], node->index, node->index,
                    node->rate, node->index);
        }
        fprintf(output_file,
                ";\nt_%d_0 = (move_%d_%d, infty).",
                node->index, node->index, node->sil.l[0]);        
        for (i = 1; i < node->sil.n; i++) {
            fprintf(output_file,
                    "t_%d\n\t+ (move_%d_%d, infty).",
                    node->index, node->index, node->sil.l[i]);
        }
        break;
    }
    if (latex) {
        sprintf(temp, "t_{%d};\\\\\n", node->index);
        strcat(process, temp);
    }
    fprintf(output_file, "t_%d;\n", node->index);
    return 0;
}

/* Recursive function which generates process definitions
   for all the leaf-nodes in the current subtree. */
void __htree_subtree_def(__htree_node_t *n) {
    __htree_child_t *c;
    short i, j;

    if (n) {
        if (n->mtype == TASK) __htree_task_def(n);
        c = n->clist;
        for (i = 0; c && i < n->nchr; i++) {
            __htree_subtree_def(c->child);
            c = c->next;
        }
    }
}

/* Generates process definitions for all the leaf-nodes
   in the skeleton hierarchy tree. */
int htree_define_tasks(void) {
    if (!__htree_rt.htree || __htree_rt.node_sum) {
        printf ("Invalid tree.\n");
        return -1;
    }
    strcpy(process, "");
    __htree_subtree_def(__htree_rt.htree);
    return 0;
}

/* Generates the overall system equation from this subtree
   of the skeleton hierarchy tree. Node the code segments
   where we define the synchronisation sets for all the
   interacting tasks under this subtree. */ 
static int set_count = 0;
int __htree_subtree_model(__htree_node_t *n) {
    static length = 0;
    __htree_child_t *c;
    int i, j, x;
    if (n) {
        if (n->mtype == TASK) {
            if (latex) {
                sprintf (temp, "t_{%d}", n->index);
                length += strlen(temp);
                strcat(model, temp);
            }
            fprintf(output_file, "t_%d", n->index);

            if (n->rank < n->p->nchr - 1) {
                if ((n->p->mtype != DEAL) &&
                    (n->p->mtype != FARM)) {
                    if (latex) {
                        sprintf(temp, "\\sync{L_{%d}}", set_count);
                        length += strlen(temp);
                        strcat(model, temp);
                        sprintf(temp, "L_{%d} & = & \\{move_{%d,%d}",
                                set_count, n->index, n->sil.l[0]);
                        strcat(set, temp);
                        for (i = 1; i < n->sil.n; i++) {
                            sprintf(temp, ", move_{%d,%d}",
                                    n->index, n->sil.l[i % n->sil.n]);
                            strcat(set, temp);
                        }
                        sprintf(temp, "\\}\\\\");
                        strcat(set, temp);
                        set_count++;
                    }
                    fprintf(output_file, " <move_%d_%d",
                            n->index, n->sil.l[0]);
                    for (i = 1; i < n->sil.n; i++)
                        fprintf(output_file, ", move_%d_%d",
                                n->index, n->sil.l[i % n->sil.n]);
                    fprintf(output_file, "> ");
                } else {
                    if (latex) {
                        sprintf(temp, "||");
                        strcat(model, temp);
                    }
                    fprintf(output_file, " || ");
                }
            }
        } else {
            if (latex) {
                sprintf(temp, "(");
                length += strlen(temp);
                strcat(model, temp);
            }
            fprintf(output_file, "(");
            c = n->clist;
            for (i = 0; c && i < n->nchr; i++) {
                __htree_subtree_model(c->child);
                c = c->next;
            }
            if (latex) {
                strcat(model, ")");
            }
            fprintf(output_file, ")");
            if (n->p) {
                if (n->rank < n->p->nchr - 1) {
                    if ((n->p->mtype != DEAL) &&
                        (n->p->mtype != FARM)) {
                        strcpy(temp_sset, " <");
                        if (latex) {
                            sprintf(temp, "\\sync{L_{%d}}", set_count);
                            strcat(model, temp);
                            length += strlen(temp);
                            sprintf(temp, "L_{%d} & = & \\{", set_count);
                            strcat(set, temp);

                            for (i = 0; i < n->sil.n; i++) {
                                x = n->sil.l[i % n->sil.n];
                                for (j = 0; j < __htree_rt.sstab[x]->sol.n; j++) {
                                    sprintf(temp, "move_{%d,%d}, ",
                                            __htree_rt.sstab[x]->sol.l[j], x);
                                    strcat(set, temp);
                                }
                            }
                            i = strlen(set);
                            set[i - 2] = '\0';
                            sprintf(temp, "\\}\\\\");
                            strcat(set, temp);
                            set_count++;
                        }
                        for (i = 0; i < n->sil.n; i++) {
                            x = n->sil.l[i % n->sil.n];
                            for (j = 0; j < __htree_rt.sstab[x]->sol.n; j++) {
                                sprintf(temp, "move_%d_%d, ",
                                        __htree_rt.sstab[x]->sol.l[j], x);
                                strcat(temp_sset, temp);
                            }
                        }
                        i = strlen(temp_sset);
                        temp_sset[i - 2] = '\0';
                        strcat(temp_sset, "> ");
                        fprintf(output_file, "%s", temp_sset);
                    } else {
                        if (latex) {
                            sprintf(temp, "||");
                            strcat(model, temp);
                        }
                        fprintf(output_file, " || ");
                    }
                }
                if (length > 100) {
                    length = 0;
                    strcat(model, "\\\\&&");
                }
            }
        }
    }
}

/* Generates the system equation for the entire
   skeleton hierarchy tree. */
int htree_define_model(void) {
    if (!__htree_rt.htree || __htree_rt.node_sum) {
        printf ("Invalid tree.\n");
        return -1;
    }
    strcpy(model, "");
    strcpy(set, "");
    __htree_subtree_model(__htree_rt.htree);
    fprintf(output_file, "\n");
    return 0;
}

/* Generate a LaTeX file which contains the performance model
   for pretty printing. */
char pepa_dot_sty[] = "\n\n\%\% Contents of pepa.sty\n\\def\\S{\\mbox{\\large $\\rhd\\!\\!\\!\\lhd$}}\\def\\Aa{\\vec{\\cal A}{\\it ct}}\\def\\cA{{\\cal A}}\\def\\cS{{\\cal S}}\\def\\cC{{\\cal C}}\\def\\cE{{\\cal E}}\\def\\cR{{\\cal R}}\\def\\Ac{{\\cal A}{\\it ct}}\\def\\bms{\\{\\!|\\,}\\def\\ems{\\,|\\!\\}}\\def\\lra{\\longrightarrow}\\def\\lera{\\leftrightarrow}\\def\\vlra{-\\hspace{-0.2cm}-\\hspace{-0.2cm}\\lra}\\def\\notsim{\\sim \\hspace{-3.5mm} /\\;}\\def\\notequiv{\\equiv \\hspace{-3.5mm} /\\;}\\def\\noapprox{\\approx \\hspace{-3.5mm} /\\;}\\def\\rmdef{\\stackrel{\\mbox{\\em {\\tiny def}}}{=}}\\def\\eq{\\mbox{\\boldmath $=$}}\\def\\Chi{\\mbox{\\Large $\\chi$}}\\def\\E{\\cC/{\\cong}}\\def\\Eup{\\cC/({\\cong} \\cR^{*} {\\cong})}\\def\\mscup{\\uplus}\\def\\mscap{\\cap}\\def\\fcomp{\\raisebox{0.6ex}{\\mbox{\\tiny $\\circ$}\\,}}\\newfont{\\cmexx}{cmex7}\\newcommand{\\smallrhd}{\\mathrel{\\raise23pt\\hbox{\\cmexx\\symbol{}}}}\\newcommand{\\smalllhd}{\\mathrel{\\raise23pt\\hbox{\\cmexx\\symbol{}}}}\\def\\smallS{\\mbox{\\tiny $\\rhd \\!\\!\\!\\lhd$}}\\newcommand{\\ssync}[1]{\\raisebox{-0.9ex}{$\\:\\stackrel{\\smallS}{\\scriptscriptstyle #1}\\,$}}  \\renewcommand{\\infty}{\\top}\\mathchardef\\infinity=\"0231\\def\\QED {{\\unskip\\nobreak\\hfil\\penalty50  \\hskip2em\\hbox{}\\nobreak\\hfil$\\Box$  \\parfillskip=0pt \\finalhyphendemerits=0 \\par}}\\def\\separate{\\begin{center} ---\\hspace{-0.12mm}---\\hspace{-0.12mm}---\\hspace{-1.5mm}$\\circ$\\hspace{-1.5mm}---\\hspace{-0.12mm}---\\hspace{-0.12mm}--- \\end{center}}  \\newcommand{\\mult}[2]{m_{#1}(#2)}\\newcommand{\\sync}[1]{\\raisebox{-1.0ex}{$\\;\\stackrel{\\S}{\\scriptscriptstyle#1}\\,$}}  \\newcommand{\\seq}[1]{\\stackrel{\\rhd}{\\scriptscriptstyle #1}}\\newcommand{\\equ}[1]{\\stackrel{#1}{\\eq}}\\newcommand{\\mat}[1]{\\mbox{\\bf #1}}\\newcommand{\\NIL}{\\mbox{{\\bf 0}}}";

void htree_write_latex(void) {
    FILE *f;
    char temp[64];
    strcpy(temp, fname);
    strcat(temp, "tex");
    f = fopen(temp, "w");
    htree_write_tree(f);
    fprintf(f, "\\documentclass[a4paper,11pt]{article}\n"
            "\\usepackage{amssymb,epsfig,fullpage}"
            "%s" /* Print contents of pepa.sty */
            "\n\n\\begin{document}\n", pepa_dot_sty);
    fprintf(f, "\\begin{figure*}\\centering"
            "\\epsfig{file=%seps,scale=0.7}\\end{figure*}",
            fname, process);
    fprintf(f, "\\begin{eqnarray*}%s\\end{eqnarray*}", process);
    fprintf(f, "\\begin{eqnarray*}model "
            " & \\rmdef & %s\\end{eqnarray*}", model);
    fprintf(f, "\\begin{eqnarray*}%s\\end{eqnarray*}", set);
    fprintf(f, "\n\n\\end{document}\n");
    fclose(f);
}

/* Generate the performance model based on the user provided
   hierarchical description to the corresponding .dot, .tex etc.
   files depending on what the user requested. */
int generate(void) {
    char temp[64];
    char command[64];
    int status;
    strcpy(temp, fname);
    strcat(temp, "pepa");

    htree_commit(); /* Commit skeleton hierarchy tree. */
    
    /* While debugging, it is easier to check the
       source-sink lookup table. */
    /*     __htree_display_sstab();     */

    if (output) {
        if (!(output_file = fopen(temp, "w"))) {
            perror("Could not create output file");
            return 1;
        }
    } else output_file = stdout;
    htree_define_tasks();
    htree_define_model();
    fclose(output_file);
    if (latex) htree_write_latex();
    if (graph) htree_write_graph();
    htree_final();    /* Finalise skeleton library. */

    /* If complete generation was requested. */
    if (complete) {
        sprintf(command, "dot -Tps -o %seps %sdot",
                fname, fname);
        switch(fork()) {
        case -1:
            perror("Cannot fork dot command");
            return -1;
        case 0:
            execl("/bin/sh", "sh", "-c", command, NULL);
        }
        if (wait(&status) < 0) return -1;
        if (!WIFEXITED(status)) return -1;

        sprintf(command, "latex %stex 1>/dev/null",
                fname);
        switch(fork()) {
        case -1:
            perror("Cannot fork latex command");
            return -1;
        case 0:
            execl("/bin/sh", "sh", "-c", command, NULL);
        }
        if (wait(&status) < 0) return -1;
        if (!WIFEXITED(status)) return -1;        

        sprintf(command, "dvips -o %sps %sdvi 2>/dev/null",
                fname, fname);
        switch(fork()) {
        case -1:
            perror("Cannot fork dvips command");
            return -1;
        case 0:
            execl("/bin/sh", "sh", "-c", command, NULL);
        }
        if (wait(&status) < 0) return -1;
        if (!WIFEXITED(status)) return -1;        
    }
    return 0;
}


/* for a description of the following function, see "pepa.h". */
int __htree_generate_sstab (__htree_node_t *n, int source, int sink) {
    __htree_child_t *c;
    short i;
    if (!n || __htree_rt.node_sum) return -1;
    if (!(__htree_rt.sstab = (__htree_node_t **)
          malloc(sizeof (__htree_node_t *)*__htree_rt.nleaves)))
        return -1;
    if (!(n->sol.l = (int *) malloc(sizeof(int)))) {
        free(__htree_rt.sstab);
        return -1;
    }
    if (!(n->sil.l = (int *) malloc(sizeof(int)))) {
        free(n->sol.l);
        free(__htree_rt.sstab);
        return -1;
    }
    
    /* Initialise the source and sink for the root node. */
    n->sol.n = 1;
    n->sol.l[0] = source;
    n->sil.n = 1;
    n->sil.l[0] = sink;
    n->temp.l = NULL;
    n->ptype = UNKNOWN;
    n->stype = UNKNOWN;
    if (n->mtype == TASK) {
        __htree_rt.sstab[n->index] = n;
        return 0;
    }
    
    /* Generate the source and sink for the remaining nodes. */
    if ((c = n->clist))
        for (i = 0; c && i < n->nchr; i++) {
            if (c->child)
                __htree_generate_source(c->child);
            c = c->next;
        }
    if (n->clist) {
        c = n->clist->prev;
        for (i = 0; c && i < n->nchr; i++) {
            if (c->child)
                __htree_generate_sink(c->child);
            c = c->prev;
        }
    }
    return 0;
}

void __htree_destroy(__htree_node_t *n) {
    __htree_child_t *c, *t;

    if (n) {
        if ((c = n->clist)) {
            c->prev->next = NULL; /* Break double linked list. */
            while (c) {
                t = c->next;
                __htree_destroy(c->child);
                free(c);
                c = t;
            }
        }
        free(n->temp.l);
        free(n->sol.l);
        free(n->sil.l);
        free(n);
    }
}

int __htree_insert_node (__htree_comp_t skel, int nchild, ...) {
    __htree_node_t *n;
    va_list ap;

    if (!(n = (__htree_node_t *) malloc(sizeof(__htree_node_t))))
        return -1;
    if ((n->mtype = skel) == TASK) {
        va_start(ap, nchild);
        strcpy(n->name, va_arg(ap, char*));
        n->rate = va_arg(ap, double);
        va_end(ap);
    } else strcpy(n->name, "unknown");

    n->clist = NULL;            /* No children yet. */
    if (__htree_rt.htree) {
        /* If there are existing nodes, we append the new node as a
           child node (sibling) which can bear a subtree of its
           own. If no nodes already exists, this node becomes the root
           node, and the parent of all. */
        switch (skel) {
        case TASK:
            /* If the node is a stage (or function) node, there are
               couple of things we have to ensure for its proper
               execution. First, we have to ensure that the data paths
               are satisfied. In order to do this, we inherit the left
               and right pointers from the parent node. */
            n->p = __htree_rt.curr_node;

            /* We have to note that this node is not allowed to have
               any children. */
            n->nchr = n->nchx = 0;

            /* Finally, we have to insert this node as one of the child
               nodes for the parent node. Remember, we maintain the
               children list as a doubl-linked list for easy navigation. */
            __htree_insert_sibling(n);
            n->index = __htree_rt.nleaves;
            __htree_rt.nleaves++;
            break;
        case PIPE:
        case DEAL:
        case FARM:
            n->p = __htree_rt.curr_node;
            n->nchr = nchild;
            n->nchx = 0;
            __htree_insert_sibling(n);
            n->index = __htree_rt.nnodes;            
            /* Now, we have to enter into this node to complete its
               subtree. Once all the required children are created,
               we return to parent. */
            __htree_rt.curr_node = n;
            break;
        default:
            printf ("Node type not recognized.\n");
        }
    } else {
        /* This creates the first nodes in the skeleton tree. This
           node, which in effect is the 'root' of the tree, won't have
           a parent node. Because each node is considered as a
           'blackbox' data processing unit, both the left and right
           subtrees are initialised with 'NULL', which means that the
           data source and sink of this node is virtual memory. */
        n->p = NULL;

        /* The 'curr_node' pointer will be left unchanged until all
           of its child nodes have been inserted. With every child
           node that is inserted, the 'nchx' is incremented by one,
           and checked against 'nchr'. If they are the same, the
           'curr_node' pointer is moved to the parent node. */
        n->nchr = nchild;
        n->nchx = 0;
        
        /* At the start, the current node and the main tree is
           initialised to this single node. */
        __htree_rt.htree = __htree_rt.curr_node = n;
    }
    __htree_rt.nnodes++;

    /* For every node (starting from the root), all the required
       children should be created. This is necessary for the tree to be
       functional because the program cannot execute unless all the
       functional units (TASKS) are supplied. Because checking the
       validity of the tree might be expensive if we have to travel the
       tree node, we account for the creation of nodes and siblings
       through this variable. For a valid tree, 'node_sum' equals
       zero. */
    __htree_rt.node_sum += nchild;

    /* Check if the required number of children have already been
       created. If this condition is satisfied, the subtree for the
       current node is complete. So, move to the next sibling. If there
       are no more siblings remaining, return to the parent. */
    while (__htree_rt.curr_node &&
           (__htree_rt.curr_node->nchr == __htree_rt.curr_node->nchx)) {
        __htree_rt.curr_node = __htree_rt.curr_node->p;
    }
    return 0;
}

int htree_commit(void) {
    /* Generate the source-sink lookup table. */
    __htree_generate_sstab(__htree_rt.htree, SOURCE_MEM, SINK_MEM);
    return 0;
}

int htree_final(void) {
    __htree_destroy(__htree_rt.htree);
    free(__htree_rt.sstab);
    return 0;
}
