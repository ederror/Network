/*
    File     : linkstate_20161606.cc
    Date     : 2021.05.16
    Author     : Minbo Shim
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NODE 100
#define MAX_MSG 1000
#define INF 999999

typedef struct{
    int nextHop[MAX_NODE];
    int dist[MAX_NODE];
}RoutingTable;


void updateTable(RoutingTable*, int[][MAX_NODE], int);
int findMinIdx(int*, int*, int);
int main(int argc, char *argv[]){

    FILE *topologyFP, *messageFP, *changesFP, *outputFP;
    int graph[MAX_NODE][MAX_NODE];
    int path[MAX_NODE];
    int numOfNode, srcNode, dstNode, cost;
    char message[MAX_MSG];
    RoutingTable rt[MAX_NODE];

    if(argc != 4){
        fprintf(stderr, "usage: linkstate topologyfile messagefile changesfile\n");
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

    // initialize graph.
    for(int i=0; i<numOfNode ; i++)
        for(int j=0; j<numOfNode; j++)
            graph[i][j] = 0;
    
    while(fscanf(topologyFP, "%d %d %d", &srcNode, &dstNode, &cost) != EOF){
        graph[srcNode][dstNode] = cost;
        graph[dstNode][srcNode] = cost;
    }

    outputFP = fopen("output_ls.txt", "w");
    
    while(1){
        updateTable(rt, graph, numOfNode);
        
        // File Write (current routing table)
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
        
        if(cost == -999){   // disconnected.
            graph[srcNode][dstNode] = 0;
            graph[dstNode][srcNode] = 0;
        }
        else{               // cost value changed
            graph[srcNode][dstNode] = cost;
            graph[dstNode][srcNode] = cost;
        }
    }
    fclose(outputFP);
}

void updateTable(RoutingTable* rt, int graph[][MAX_NODE], int numOfNode){
    int v, w, u; // node
    int visited[MAX_NODE];
    
    // Initialize routing table.
    for(int i=0 ; i < numOfNode ; i++){
        for(int j=0; j < numOfNode ; j++){
            rt[i].dist[j] = INF;
            rt[i].nextHop[j] = -1;
        }
        rt[i].dist[i] = 0;
        rt[i].nextHop[i] = i;
    }

    // do Dijkstra's algorithm on node v.
    for (v=0; v<numOfNode ; v++){
        
        // Init visited list
        for (int i=0 ; i<numOfNode ; i++)
            visited[i] = 0;

        // Loop n times.
        for (int i=0 ; i < numOfNode ; i++){
            u = findMinIdx(rt[v].dist, visited, numOfNode);
            
            if(u == -1) // not found.
                break;
            visited[u] = 1;
            
            // for all adjacent node w
            for(w = 0; w < numOfNode ; w++){
                
                // if not adjacent
                if(graph[u][w] == 0)
                    continue;
                
                // update dist
                if(visited[w] == 0 && rt[v].dist[w] > rt[v].dist[u] + graph[u][w]){
                    rt[v].dist[w] = rt[v].dist[u] + graph[u][w];
                    
                    // rt[v].nextHop[w]
                    if(rt[v].nextHop[u] == v)
                        rt[v].nextHop[w] = w;
                    else
                        rt[v].nextHop[w] = rt[v].nextHop[u];
                }
            }
        }
    }
    
    // Special case : graph is seperated.
    for(int i=0 ;i < numOfNode ; i++)
        for(int j=0; j<numOfNode ; j++)
            if(rt[i].nextHop[j] == -1){
                rt[i].nextHop[j] = 0;
                rt[i].dist[j] = 0;
            }
    
    return;
}

/*
    Find the element in arr
    which has minimum value and has not been visited yet.
 */
int findMinIdx(int* arr, int* visited, int len){
    int idx = 0, min = INF;
    for(int i=0; i < len ; i++){
        if(visited[i] == 0 && arr[i] < min){
            min = arr[i];
            idx = i;
        }
    }
    
    if(visited[idx] == 0)
        return idx;
    else
        return -1;
}
