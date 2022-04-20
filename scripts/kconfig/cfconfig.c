
#define _GNU_SOURCE
#include <assert.h>
#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "configfix.h"

static struct symbol_dvalue * sym_create_sdv(struct symbol *sym, char *input);
static void print_diagnoses_symbol(struct sfl_list *diag_sym);
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


static int fix_config(struct symbol *sym, char *newval)
{
        struct sfl_list *diagnoses;
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
                /* print_diagnoses_symbol(diagnoses); */
                sfl_list_for_each(node, diagnoses){
                        apply_fix(node->elem);
                        print_diagnosis_symbol(node->elem);
                        /* printd("\nResetting config.\n"); */
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

/*
 * print the diagnoses of type symbol_fix
 */
/* static void print_diagnoses_symbol(struct sfl_list *diag_sym) */
/* { */
/* 	struct sfl_node *arr; */
/* 	unsigned int i = 1; */

/* 	sfl_list_for_each(arr, diag_sym) { */
/* 		printd(" %d: ", i++); */
/* 		print_diagnosis_symbol(arr->elem); */
/* 	} */
/* } */
