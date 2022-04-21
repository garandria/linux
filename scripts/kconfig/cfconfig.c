
#define _GNU_SOURCE
#include <assert.h>
#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "configfix.h"

static struct symbol_dvalue * sym_create_sdv(struct symbol *sym, char *input);
/* -------------------------------------- */

static struct symbol *get_random_sym(void)
{
        int ri;
        struct timeval now;
        struct symbol *sym;

        /*
         * Use microseconds derived seed, compensate for systems where it may be
         * zero.
         */
        gettimeofday(&now, NULL);
        srand((now.tv_sec + 1) * (now.tv_usec + 1));

        for (ri = rand()%SYMBOL_HASHSIZE, sym = symbol_hash[ri];
             (sym == NULL) ||
                     (sym_get_type(sym) != S_BOOLEAN
                      && sym_get_type(sym) != S_TRISTATE);
             ri = rand()%SYMBOL_HASHSIZE, sym = symbol_hash[ri]);

        printf("random symbol: %s %s %s\n",
               sym_type_name(sym->type), sym->name,
               sym_get_string_value(sym));

        return sym;
}

static int same_symbol_fix(struct symbol_fix *x, struct symbol_fix *y)
{
        return !(strcmp(sym_get_name(x->sym), sym_get_name(y->sym)))
                && x->tri == y->tri;
}

static int same_sfix_node(struct sfix_node *x, struct sfix_node *y)
{
        return same_symbol_fix(x->elem, y->elem);
}

static int same_sfix_list(struct sfix_list *l1, struct sfix_list *l2)
{
        struct sfix_node *n1, *n2;
        int found;

        if (l1->size != l2->size)
                return -1;

        sfix_list_for_each(n1, l1) {
                found = -1;
                sfix_list_for_each(n2, l2) {
                        if (same_sfix_node(n1, n2)) {
                                found = 1;
                                break;
                        }
                }
                if (!found)
                        return found;
        }
        return 1;
}

static int same_sfl_node(struct sfl_node *x, struct sfl_node *y)
{
        return same_sfix_list(x->elem, y->elem);
}

static struct sfl_list *sfl_list_remove_duplicate(struct sfl_list *l)
{
        struct sfl_node *n, *nn;
        struct sfl_list *res = sfl_list_init();
        sfl_list_add(res, l->head->elem);

        sfl_list_for_each(n, l) {
                sfl_list_for_each(nn, res) {
                        if (!same_sfl_node(n, nn)) {
                                sfl_list_add(res, n->elem);
                                break;
                        }
                }
        }
        return res;
}

static int fix_config(struct symbol *sym, char *newval)
{
        struct sfl_list *diagnoses, *diagnoses_wodup;
        struct sdv_list *symbols;
        char buf[256];
        const char *currval;
        struct symbol_dvalue *sdv;
        struct sfl_node *node;
        unsigned int counter;

        CFDEBUG = true;

        symbols = sdv_list_init();

        sdv = sym_create_sdv(sym, newval);
        currval = sym_get_string_value(sym);

        if (!(strcmp(currval, newval))){
                printd("%s already set to %s\n", sym->name, currval);
                return 0;
        }

        printd("Change: %s (%s) %s -> %s\n",
               sym->name, sym_type_name(sym->type), currval, newval);

        sdv_list_add(symbols, sdv);

        diagnoses = run_satconf(symbols);

        counter = 0;
        if (diagnoses == NULL) {
                printd("-> Everything is OK for the change\n");
                printd("\tsym_calc_value: update symbol's new value\n");
                printf("\t\\_\tvalue (newval=%s): %s ->",
                       newval, sym_get_string_value(sym));
                sym_set_string_value(sym, newval);
                sym_calc_value(sym);
                printf(" %s\n", sym_get_string_value(sym));
                sprintf(buf, "___config%s_%s-%d", sym->name, newval, counter);
                if (conf_write(buf) < 0)
                        return -1;
                return 1;
        }

        if (diagnoses->size == 0) {
                printd("-> No diagnoses FOUND\n");
                return -1;
        } else {
                diagnoses_wodup = sfl_list_remove_duplicate(diagnoses);
                sfl_list_for_each(node, diagnoses_wodup){
                        apply_fix(node->elem);
                        print_diagnosis_symbol(node->elem);
                        sprintf(buf, "___config%s_%s-%d", sym->name, newval, counter++);
                        if (conf_write(buf) < 0) {
                                printd("Error writing configuration %s\n", buf);
                                return -1;
                        }
                        if (conf_read(NULL) < 0) {
                                printd("Error conf_read\n");
                                return -1;
                        }
                }
        }
        return counter;
}


        }

        return EXIT_SUCCESS;
}

/*
 * create a symbol_dvalue struct containing the symbol and the desired value
 */
static struct symbol_dvalue * sym_create_sdv(struct symbol *sym, char *input)
{
	struct symbol_dvalue *sdv = malloc(sizeof(struct symbol_dvalue));
	sdv->sym = sym;
	sdv->type = sym_is_boolean(sym) ? SDV_BOOLEAN : SDV_NONBOOLEAN;

	if (sym_is_boolean(sym)) {
		if (strcmp(input, "y") == 0)
			sdv->tri = yes;
		else if (strcmp(input, "m") == 0)
			sdv->tri = mod;
		else if (strcmp(input, "n") == 0)
			sdv->tri = no;
		else
			perror("Not a valid tristate value.");

		/* sanitize input for booleans */
		if (sym->type == S_BOOLEAN && sdv->tri == mod)
			sdv->tri = yes;
	} else if (sym_is_nonboolean(sym)) {
		sdv->nb_val = str_new();
		str_append(&sdv->nb_val, input);
	}

	return sdv;
}
