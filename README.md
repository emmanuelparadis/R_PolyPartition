
- [R_PolyPartition](#r_polypartition)
  - [Requirements](#requirements)
  - [A few details on the port to R](#a-few-details-on-the-port-to-r)
  - [Installation](#installation)
  - [A simple example](#a-simple-example)
  - [Polygon without hole](#polygon-without-hole)
  - [Polygon with hole](#polygon-with-hole)
  - [Further comments](#further-comments)

# R_PolyPartition

This is a small experiment to port some C++ code to R. The code is from
this GH repository: <https://github.com/ivanfratric/polypartition>.

This C++ library performs partitions of polygons: triangulation under
various methods, and partition into convex polygons. The input polygons
can include holes (see below and the above link for details).

Triangulation of polygons is also implemented in the R package
[tigers](https://github.com/emmanuelparadis/tigers) but with only two
methods (see examples below).

## Requirements

- R with a C/C++ compiler (properly configured to compile code from R
  commands). This is standard on Linux; for other platforms, see the
  [documentation on R](https://cran.r-project.org/manuals.html).
- These two files: `polypartition.cpp` and `polypartition.h` (from
  [HERE](https://github.com/ivanfratric/polypartition/tree/master/src)).
- The file `R_polypartition.cpp` distributed in this repository.

## A few details on the port to R

Calling C++ code from R necessitates to write a driver in C that passes
the data from R to the C++ code. This must be wrapped into a
`extern "C"` command like this:

``` c
extern "C" {

SEXP Call_Triangulate(SEXP XY, SEXP METHOD)
{
....
}// end of Call_Triangulate
}// end of extern "C"
```

Additional C code can be written outside this; typically, these will
include functions to transfer the data from R objects (SEXP structures
in C) into the data structures defined in the C++ code.

For the present implementation, two drivers were written (see examples
below) and two functions for data conversions. Please see the code in
this repository for further details (or open an issue if you want to
open a public discussion).

## Installation

The commands below are for a Linux platform and may need to be adapted
for other platforms. We suppose the three .cpp files are in the current
directory. Then we call the following (system or shell) command to
compile them and create the file `R_polypartition.so`:

`R CMD SHLIB -c R_polypartition.cpp polypartition.cpp`

The order of the two .cpp files is important.

## A simple example

To use the code from R, we first load the shared library into an R
session:

``` r
dyn.load("R_polypartition.so")
```

We can now try to partition a square (obviously into two triangles):

``` r
X <- rbind(c(0, 0), c(1, 0), c(1, 1), c(0, 1))
res_tri <- .Call("Call_Triangulate", X, 1)
res_tri
```

    ## [[1]]
    ##      [,1] [,2]
    ## [1,]    0    1
    ## [2,]    0    0
    ## [3,]    1    0
    ## 
    ## [[2]]
    ##      [,1] [,2]
    ## [1,]    0    1
    ## [2,]    1    0
    ## [3,]    1    1

As expected, the result is a list with two triangles (two matrices with
three rows and two columns). We can visualise the results with simple
plotting commands:

``` r
plot(X, , "n", asp = 1)
polygon(X, lwd = 5)
vac <- lapply(res_tri, polygon, border = "green")
```

![](README_files/figure-gfm/unnamed-chunk-3-1.png)<!-- -->

We note here that the C++ code of PolyPartition needs to have the
polygon points (or vertices) into counter-clockwise order.

## Polygon without hole

We can now test the five triangulation methods of PolyPartition with a
more complicated example: a polygon drawing a (simplified) man with
coordinates in the file `data`. This file is borrowed from the file
`test_input.txt` from the PolyPartition repository where the hole and
extra information (numbers of points) have been removed.

``` r
X <- matrix(scan("data"), ncol = 2, byrow = TRUE)
str(X)
```

    ##  num [1:44, 1:2] 170 179 178 163 163 212 230 230 254 254 ...

Plotting `X` would show the man upside down (see [this
discussion](https://github.com/ivanfratric/polypartition/issues/38) for
details), so we mirror the y-coordinates which has the side-effect to
change the orientation of the polygon. To keep it counter-clockwise, we
use functions from tigers as follows:

``` r
library(tigers)
X[, 2] <- 400 - X[, 2] # mirror
is.clockwise(X)
```

    ## [1] TRUE

``` r
X <- revPolygon(X) # function from tigers
is.clockwise(X)
```

    ## [1] FALSE

We can now perform the triangulations and plot the results:

``` r
Methods <- c("Triangulate_EC", "Triangulate_OPT",
             "Triangulate_MONO", "ConvexPartition_HM",
             "ConvexPartition_OPT")

layout(matrix(c(1:5, 0), 2, 3, TRUE))

for (method in 1:5) {
    o <- try(.Call("Call_Triangulate", X, method))
    plot(X, , "n", asp = 1, main = Methods[method])
    polygon(X)
    if (!is.list(o)) next # in case triangulation failed
    vac <- lapply(o, polygon, col = "darkgreen", border = "white")
}
```

![](README_files/figure-gfm/unnamed-chunk-6-1.png)<!-- -->

We compare these results with those with the function `triangulate()` in
tigers. We note a difference in the way the results are returned by this
last function: a matrix with three columns giving the indices of the
vertices of each triangle; their coordinates can be found from the rows
of the input polygon.

``` r
layout(matrix(1:2, 1, 2))

for (method in 1:2) {
    tri <- triangulate(X, , method)
    plot(X, type = "n", asp = 1, main = method)
    for (i in 1:nrow(tri))
        polygon(X[tri[i, ], ], border = "white", col = "green")
}
```

![](README_files/figure-gfm/unnamed-chunk-7-1.png)<!-- -->

## Polygon with hole

We can now try the methods that allow holes in the input polygon. The
interface has been adapted to R: the hole is input as a second polygon
and both polygons are passed to C++ as a list using the second C driver
mentioned above.

``` r
h <- matrix(scan("data_hole"), ncol = 2, byrow = TRUE)
XY <- list(list(X, h))
```

Not all methods tried above support hole(s) in polygon (again, see the
original code of PolyPartition for details):

``` r
Methods <- c("Triangulate_EC", "MonotonePartition",
             "Triangulate_MONO", "ConvexPartition_HM")

layout(matrix(1:4, 2, 2, TRUE))
par(mar = c(0, 0, 1, 0))
for (method in 1:4) {
    o <- .Call("Call_Triangulate_List", XY, method)
    plot(X, , "n", asp = 1, axes = 0, ann = 0)
    title(main = Methods[method])
    polygon(X); polygon(h, col = "yellow")
    out <- lapply(o, polygon, col = "darkgreen", border = "white")
}
```

![](README_files/figure-gfm/unnamed-chunk-9-1.png)<!-- -->

## Further comments

I run other tests which results are summarised below.

- The triangulation methods Triangulate_OPT (2 in the above example) and
  Triangulate_MONO (3) have bugs: see issues
  [48](https://github.com/ivanfratric/polypartition/issues/48),
  [58](https://github.com/ivanfratric/polypartition/issues/58), and
  [59](https://github.com/ivanfratric/polypartition/issues/59) in the
  repository of PolyPartition. It is not clear whether the code of
  PolyPartition is still (actively) maintained.

- PolyPartition’s Triangulate_EC method (1 above) gives almost identical
  results than `tigers::triangulate(, method = 2)` with similar running
  times for both programs.

- `tigers::triangulate(, method = 1)` is 2 to 3 times faster than the
  previous commands. This method performs a very rough triangulation
  creating a triangle each time an ear is encountered.

- The methods ConvexPartition_HM and ConvexPartition_OPT of
  PolyPartition are slower (probably because a triangulation is first
  done).
