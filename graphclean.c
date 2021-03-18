#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <graphviz/cgraph.h>


int main (int argc, char **argv)
{
	FILE *graphf = NULL, *gimg = NULL;
	char *name, *name2;
	Agraph_t *g, *g2;
	Agnode_t *n, *n2, *n3, *nn, *nn2;
	Agedge_t *e, *e2, *ee;

	if ((graphf = fopen(argv[1], "r")) == NULL){
		printf("File not found %s\n", argv[1]);
		return ENOENT;
	}
	
	g = agread(graphf, NULL);
	g2 = agopen("ODep", Agundirected, NULL);
	
	
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		if ((e = agfstin(g, n)) != NULL){
			n2 = agtail(e);
			name = agnameof(n2);
			nn = agnode(g2, name, FALSE);
			if (nn == NULL)
				nn = agnode(g2, name, TRUE);
			for (e2 = agfstin(g, n); e2; e2 = agnxtin(g, e2)) {
				n3 = agtail(e2);
				name2 = agnameof(n3);
				nn2 = agnode(g2, name2, FALSE);
				if (nn2 == NULL)
					nn2 = agnode(g2, name2, TRUE);
				ee = agedge(g2, nn, nn2, NULL, FALSE);
				if (ee == NULL){
					ee = agedge(g2, nn, nn2, NULL, TRUE);
				}
			}
		}
	}
	agclose(g);
	
	gimg = fopen(argv[2], "w");
	agwrite(g2, gimg);
	fclose(gimg);

	agclose(g2);
	return 0;
}
