# b-suitor
Implementation of multithreaded shared memory algorithm for approximate b-suitor

Solution is based on: https://www.cs.purdue.edu/homes/apothen/Papers/bMatching-SISC-2016.pdf

## Problem statment

From the abstract of Efficient approximation algorithms for weighted b-Matching (Khan, Arif, et al, 2016):

> b-Matching is a generalization of the well-known Matching problem in graphs, where the objective is to choose a subset of M
> edges in the graph such that at most a specified number b(v) of edges in M are incident on each vertex v. Subject to this 
> restriction we maximize the sum of the weights of the edges in M.

## Pseudo-code
Input: A graph G = (V, E, w) and a vector b. Output: A 1/2−approximate edge weighted b-Matching M.
 ```python
 procedure Parallel b-Suitor(G, b):
  Q = V ; Q'= {}; 
  while Q is not empty:
    for all vertices u in Q in parallel:
      i = 1;
      while i <= b(u) and N(u) != exhausted:
        Let p in N(u) be an eligible partner of u;
        if p != NULL:
          Lock p; 
          if p is still eligible:
            i = i + 1; 
            Make u a Suitor of p;
            if u annuls the proposal of a vertex v:
              Add v to Q';
              Update db(v); 
          Unlock p; 
        else:
          N(u) = exhausted; 
    Update Q using Q';
    Update b using db;
```
## Code optimization
- partly sorting - reduces the number of edge traversals
- own spin lock instead of mutex -in that case spin lock are faster
- eager update of rejected nodes

## Performance tests

Tested on: https://snap.stanford.edu/data/soc-pokec.html 

| Thread number | 1       | 2       |  3      |  4       | 5       | 6       | 7       | 8       |
| ------------- | ------- | ------- | ------- | -------- | ------- | ------- | ------- | ---- |
| time          | 29.7978 | 20.4423 | 15.5542 |  12.4749 | 11.9792 | 10.4477 | 8.97703 | 8.00229 |
| speed up      | | 3.72365 | 3.31933 | 2.85209 | 2.48746 | 2.388620 | 1.91573 | 1.45765 |  
