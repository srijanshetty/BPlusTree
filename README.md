# B+ Tree

## Results

- The computed time is in microseconds.

QUERY | MAX     | MIN | AVG     | STD
0     | 4177465 | 95  | 208859  | 20874.2
1     | 389     | 57  | 817.42  | 75.042
2     | 534     | 58  | 1047.53 | 95.453
3     | 1025    | 71  | 1856.51 | 170.451
4     | 1078    | 60  | 1898.71 | 173.971

## Observations

- The maximum time taken by an insertion is huge compared to the maximum times
of other operations. An insertion can start a chain of insertions when a child
delegates an insertion to its parent, there by accounting for the large maximum.

- The huge standard deviation in insertions is accounted by the fact that not
all insertions will cascade into parent insertions.

- Point Queries take the least amount of time because they only need a single
pass through the tree.

- Due to the query bias - range queries had a very small radius, the times reported
for range queries seem to be very small. This shouldn't be the case as internally
rangeQuery calls windowQuery.

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

- There are two debug levels in the program: DEBUG_NORMAL and DEBUG_VERBOSE.
