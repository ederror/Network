/*
	File 	: distvec_20161606.cc
	Date 	: 2021.05.12
	Author 	: Minbo Shim
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NODE 100
#define MAX_MSG 1000

typedef struct{
    int nextHop[MAX_NODE];
	int dist[MAX_NODE];
}RoutingTable;


void updateTable(RoutingTable*, int[][MAX_NODE], int);
int main(int argc, char *argv[]){

	FILE *topologyFP, *messageFP, *changesFP, *outputFP;
	int graph[MAX_NODE][MAX_NODE];
	int path[MAX_NODE];
	int numOfNode, srcNode, dstNode, cost;
	char message[MAX_MSG];
	RoutingTable rt[MAX_NODE];

	if(argc != 4){
		fprintf(stderr, "usage: distvec topologyfile messagefile changesfile\n");
		exit(1);
	}

	// File open
	if((topologyFP = fopen(argv[1], "r")) == NULL){
		fprintf(stderr, "Error: open input file.\n");
		exit(1);
	}
	if((messageFP = fopen(argv[2], "r")) == NULL){
		fprintf(stderr, "ERROR: open input file.\n");
		exit(1);
	}
	if((changesFP = fopen(argv[3], "r")) == NULL){
		fprintf(stderr, "ERROR: open input file.\n");
		exit(1);
	}

	// File read - topologyfile
	fscanf(topologyFP, "%d", &numOfNode);

	// initialize graph & routing table
	for(int i=0; i<numOfNode ; i++){
		for(int j=0; j<numOfNode; j++){
			graph[i][j] = 0;
			rt[i].dist[j] = 0;
			rt[i].nextHop[j] = 0;
		}
		rt[i].nextHop[i] = i;
	}

	// initialize Routing table.
	while(fscanf(topologyFP, "%d %d %d", &srcNode, &dstNode, &cost) != EOF){
		graph[srcNode][dstNode] = cost;
		graph[dstNode][srcNode] = cost;

		// src->dst
		rt[srcNode].dist[dstNode] = cost;
		rt[srcNode].nextHop[dstNode] = dstNode;
		// dst->src
		rt[dstNode].dist[srcNode] = cost;
		rt[dstNode].nextHop[srcNode] = srcNode;
	}

	outputFP = fopen("output_dv.txt", "w");
	
	while(1){
		updateTable(rt, graph, numOfNode);
		// File Write
		for(int i=0 ; i<numOfNode; i++){
			for(int j=0 ; j<numOfNode; j++){
				if(i != j && rt[i].dist[j] == 0)
					continue;
				fprintf(outputFP, "%d %d %d\n", j, rt[i].nextHop[j], rt[i].dist[j]);
			}
			fprintf(outputFP, "\n");
		}

        // Read message file and find the path.
		fseek(messageFP, 0 , SEEK_SET); // stream reset
		while(fscanf(messageFP, "%d %d", &srcNode, &dstNode) != EOF){
			fgets(message, MAX_MSG, messageFP);
			
			int i=0, tmp = srcNode;
		 	for(i=0 ; i<numOfNode ; i++){
				if(tmp == dstNode || rt[tmp].dist[dstNode] == 0)
					break;
				path[i] = tmp;
				tmp = rt[tmp].nextHop[dstNode];
			}

            // Write the result.
			if(tmp == dstNode){
				fprintf(outputFP, "from %d to %d cost %d hops ",
						srcNode, dstNode, rt[srcNode].dist[dstNode]);
				for(int idx=0; idx<i; idx++)
					fprintf(outputFP, "%d ", path[idx]);
				fprintf(outputFP, "message %s", message);
			}
			else{
				fprintf(outputFP, "from %d to %d cost infinite hops unreachable message %s\n", 
						srcNode, dstNode, message);
			}
		}
        fprintf(outputFP, "\n");

        // Read changes and change the graph[][]
		if(fscanf(changesFP, "%d %d %d", &srcNode, &dstNode, &cost) == EOF)
			break;
		
		if(cost == -999){
			graph[srcNode][dstNode] = 0;
			graph[dstNode][srcNode] = 0;
			rt[srcNode].dist[dstNode] = 0;
			rt[srcNode].nextHop[dstNode] = 0;
			rt[dstNode].dist[srcNode] = 0;
			rt[dstNode].nextHop[srcNode] = 0;
		}
		else{
			graph[srcNode][dstNode] = cost;
			graph[dstNode][srcNode] = cost;
			rt[srcNode].dist[dstNode] = cost;
			rt[srcNode].nextHop[dstNode] = dstNode;
			rt[dstNode].dist[srcNode] = cost;
			rt[dstNode].nextHop[srcNode] = srcNode;
		}
	}
	fclose(outputFP);
}

void updateTable(RoutingTable* rt, int graph[][MAX_NODE], int numOfNode){
	int updateCount;

	// Initialize routing table.
	for(int i=0 ; i < numOfNode ; i++){
		for(int j=0; j < numOfNode ; j++){
			// src->dst
			rt[i].dist[j] = graph[i][j];
			rt[i].nextHop[j] = j;
			// dst->src
			rt[j].dist[i] = graph[i][j];
			rt[j].nextHop[i] = i;

		}
		rt[i].nextHop[i] = i;
	}

	// Exchange routing table & update
	do{
		updateCount = 0;
		
		// update i-th node's routing table.
		for(int i = 0 ; i < numOfNode ; i++){

			// update rt[i].dist[j] (i->j)
			for(int j = 0 ; j < numOfNode ; j++){
				if(i==j) continue;

				for(int k = 0 ; k < numOfNode ; k++){
					if(rt[i].dist[k] == 0 || rt[k].dist[j] == 0)
						continue;
					// i->j > i->k + k->j
					if(rt[i].dist[j] == 0
						|| rt[i].dist[j] > rt[i].dist[k]+rt[k].dist[j]){
						rt[i].dist[j] = rt[i].dist[k] + rt[k].dist[j];
						rt[i].nextHop[j] = rt[i].nextHop[k];
						updateCount++;
					}
	
					// Tie-breaking rule 1.
					if(rt[i].dist[j] == rt[i].dist[k]+rt[k].dist[j]
							&& rt[i].nextHop[j] > rt[i].nextHop[k]){
						rt[i].nextHop[j] = rt[i].nextHop[k];
						updateCount++;		
					}
				}
			}
		}
	}while(updateCount != 0);	// Break if there is no more update.

	return;
}
