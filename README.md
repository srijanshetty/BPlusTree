# B+ Tree

## Results
- The computed time is in microseconds.

### Page Size : 2048kB

QUERY | MAX    | MIN | AVG     | STD
0     | 198110 | 71  | 14482.1 | 1440.71
2     | 302    | 38  | 563.82  | 50.382
4     | 233    | 39  | 617.12  | 56.312

## Observations

### Insertion vs Other Queries
- The maximum time taken by an insertion is huge compared to the maximum times
of other operations. An insertion can start a chain of insertions when a child
delegates an insertion to its parent, there by accounting for the large maximum.

### Huge Standard Deviation in Insertions
- The huge standard deviation in insertions is accounted by the fact that not
all insertions will cascade into parent insertions.

### Point Queries
- Point Queries take the least amount of time because they only need a single
pass through the tree.

### Range Queries vs Other Queries
- Due to the query bias - range queries had a very small radius, the times reported
for range queries seem to be very small. This shouldn't be the case as internally
rangeQuery calls windowQuery.

### Effectiveness of B+Tree
- One of the most important things to note is the time taken by all these queries is
very small for a disk based structure which shows the effectiveness of B+Tree and the
rationale for using it in Databases.

## INSTALL

- To build a new tree you can run:

```shell
$ make build
```

- To build the tree which was computed in the first run:

```shell
$ make restore
```

- To time the program the configuration is as follows:

```c++
// #define OUTPUT
#define TIME
```

- To just get the output, the configuration is as follows:

```c++
#define OUTPUT
// #define TIME
```
