#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <graphviz/cgraph.h>
#include <string.h>

void compute(Agraph_t **graph, char *name, char *color, int *cost){
	Agraph_t *g = *graph;
	Agedge_t *e, *e2;
	Agnode_t *n, *n2, *file, *feature;

	if ((n = agnode(g, name, FALSE)) == NULL){
		printf("Unknown node: %s\n", name);
		return;
	}

	if (!strcmp(agget(n, "color"), color))
		return;

	agset(n, "color", color);
	for (e = agfstout(g, n); e; e = agnxtout(g, e)){
		file = aghead(e);
		if (!strcmp(agget(file, "color"), color))
			return;
		else {
			agset(file, "color", color);
			(*cost)++;
		}
		for (e2 = agfstin(g, file); e2; e2 = agnxtin(g, e2)){
			feature = agtail(e2);
			compute(&g, agnameof(feature), color, cost);
		}
	}
}

int main (int argc, char **argv)
{
  /* Usage: ./files <OPTION_NAME> <.dot GRAPH> */
	FILE *graphf = NULL, *output = NULL;
	char *name = argv[1];
	Agraph_t *g;
	Agnode_t *n, *nc, target;
	Agedge_t *e;
	int cost = 0;
	
	if ((graphf = fopen(argv[2], "r")) == NULL){
		printf("File not found %s\n", argv[1]);
		return ENOENT;
	}
	g = agread(graphf, NULL);
	fclose(graphf);

	if ((n = agnode(g, name, FALSE)) == NULL){
		printf("Unknown node: %s\n", name);
		return -1;
	}
	
	compute(&g, name, "blue", &cost);
	
	printf("COST = %d\n", cost);
	return 0;
}
