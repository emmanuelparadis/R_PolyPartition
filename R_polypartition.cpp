// Copyright 2025 Emmanuel Paradis (2025-02-12)

#define _CRT_SECURE_NO_WARNINGS

#include <R.h>
#include <Rinternals.h>
#include <list>

using namespace std;

#include "polypartition.h"

void Rmatrix2Poly(double *x, int n, TPPLPoly *poly)
{
    double *y;
    int i;
    TPPLPoint *p;

    y = x + n;
    p = poly->GetPoints();

    for (i = 0; i < n; i++) {
	p[i].x = x[i];
	p[i].y = y[i];
    }
}

SEXP TPPLPoly2R_list(TPPLPolyList *res)
{
    SEXP obj, mat;
    double *m;
    int i, j, n;
    TPPLPoint *p;
    list<TPPLPoly>::iterator iter;

    PROTECT(obj = allocVector(VECSXP, (res)->size()));

    j = 0;
    for (iter = (res)->begin(); iter != (res)->end(); iter++) {
	n = iter->GetNumPoints();
	PROTECT(mat = allocMatrix(REALSXP, n, 2));
	m = REAL(mat);
	p = iter->GetPoints();
	for (i = 0; i < n; i++) {
	    m[i] = p[i].x;
	    m[i + n] = p[i].y;
	}
	SET_VECTOR_ELT(obj, j++, mat);
	UNPROTECT(1);
    }

    UNPROTECT(1);
    return obj;
}

extern "C" {

SEXP Call_Triangulate(SEXP XY, SEXP METHOD)
{
    TPPLPartition pp;
    int n, info;
    TPPLPoly poly;
    list<TPPLPoly> res;

    PROTECT(XY = coerceVector(XY, REALSXP));
    PROTECT(METHOD = coerceVector(METHOD, INTSXP));

    n = nrows(XY); // Nb of vertices
    (&poly)->Init(n); // initialize
    Rmatrix2Poly(REAL(XY), n, &poly); // copy the data

    switch (INTEGER(METHOD)[0]) {
    case 1 : info = pp.Triangulate_EC(&poly, &res); break;
    case 2 : info = pp.Triangulate_OPT(&poly, &res); break;
    case 3 : info = pp.Triangulate_MONO(&poly, &res); break;
    case 4 : info = pp.ConvexPartition_HM(&poly, &res); break;
    case 5 : info = pp.ConvexPartition_OPT(&poly, &res); break;
    }

    if (!info) Rf_error("procedure failed\n");
    UNPROTECT(2);
    return TPPLPoly2R_list(&res);
}

SEXP Call_Triangulate_List(SEXP XY, SEXP METHOD)
{
    TPPLPartition pp;

    SEXP obj, xy;
    int i, j, n, info;

    TPPLPoly poly;
    list<TPPLPoly> lpoly, res;

    PROTECT(XY = coerceVector(XY, VECSXP));
    PROTECT(METHOD = coerceVector(METHOD, INTSXP));

    for (i = 0; i < LENGTH(XY); i++) {
	PROTECT(obj = VECTOR_ELT(XY, i));
	for (j = 0; j < LENGTH(obj); j++) {
	    PROTECT(xy = VECTOR_ELT(obj, j));
	    n = nrows(xy);
	    (&poly)->Init(n);
	    Rmatrix2Poly(REAL(xy), n, &poly);
	    poly.SetHole((bool)j);
	    lpoly.push_back(poly);
	    UNPROTECT(1);
	}
	UNPROTECT(1);
    }

    //printf("size of lpoly: %ld\n", (&lpoly)->size());

    switch (INTEGER(METHOD)[0]) {
    case 1 : info = pp.Triangulate_EC(&lpoly, &res); break;
    case 2 : info = pp.MonotonePartition(&lpoly, &res); break;
    case 3 : info = pp.Triangulate_MONO(&lpoly, &res); break;
    case 4 : info = pp.ConvexPartition_HM(&lpoly, &res); break;
    }

    if (!info) Rf_error("procedure failed\n");
    UNPROTECT(2);
    return TPPLPoly2R_list(&res);
}

}// end of extern "C"
