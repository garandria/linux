
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

static struct symbol *get_random_sym(void) {
        int i, ri;
        struct timeval now;
        struct symbol *sym;

        /*
         * Use microseconds derived seed, compensate for systems where it may be
         * zero.
         */
        gettimeofday(&now, NULL);
        srand((now.tv_sec + 1) * (now.tv_usec + 1));

        ri = rand()%SYMBOL_HASHSIZE;
        printf("random int: %d\n", ri);
        for_all_symbols(i, sym) {
                /* if (sym_has_value(sym) || sym->flags & SYMBOL_VALID) */
                /*   continue; */
                switch (sym_get_type(sym)) {
                case S_BOOLEAN:
                case S_TRISTATE:
                        /* printf("%s", sym->name); */
                        if (i == ri){
                                printf("random symbol: %s %s %s\n",
                                       sym_type_name(sym->type), sym->name,
                                       sym_get_string_value(sym));
                                return sym;
                        }
                default:
                        continue;
                }
        }
        return NULL;
}


int main(int argc, char *argv[])
{
        struct sfl_list *diagnoses;
        struct sdv_list *symbols;
        char buf[256], *option = argv[2], *newval = argv[3];
        const char *currval;
        struct symbol *sym;
        struct symbol_dvalue *sdv;
        struct sfl_node *node;
        unsigned int counter;

        CFDEBUG = true;

        printd("CONFIGFIX-CLI\n-------------\nModified for CMUT\n");
        printd("Initialization: Kconfig, .config\n");

        conf_parse(argv[1]);

        if (conf_read(NULL)){
          printd(".config not found!\n");
          return EXIT_FAILURE;
        }

        symbols = sdv_list_init();

        if ((sym = sym_find(option)) == NULL){
                printd("Symbol %s not found\n", option);
                return EXIT_FAILURE;
        }

        sdv = sym_create_sdv(sym, newval);
        currval = sym_get_string_value(sym);

        if (!(strcmp(currval, newval))){
                printd("%s already set to %s\n", sym->name, newval);
                return EXIT_SUCCESS;
        }

        printd("Change: %s (%s) %s -> %s\n",
               sym->name, sym_type_name(sym->type), currval, newval);

        /* sym_calc_value(sym); */
        /* conf_write("___CONFIG_TEST"); */
        /* return EXIT_SUCCESS; */

        sdv_list_add(symbols, sdv);

        diagnoses = run_satconf(symbols);

        if (diagnoses->size == 0){
                printd("No diagnoses available: everything is OK for the change\n");
                printd("sym_calc_value: update symbol's new value\n");
                sym_calc_value(sym);
                sprintf(buf, "___config%s", option);
                conf_write(buf);
        } else {
                /* print_diagnoses_symbol(diagnoses); */
                counter = 1;
                sfl_list_for_each(node, diagnoses){
                        apply_fix(node->elem);
                        print_diagnosis_symbol(node->elem);
                        /* printd("\nResetting config.\n"); */
                        sprintf(buf, "___config%s-%d", option, counter++);
                        conf_write(buf);
                        conf_read(NULL);
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
