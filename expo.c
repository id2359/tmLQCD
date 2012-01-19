/***********************************************************************
 * Copyright (C) 2001 Martin Hasenbusch
 *
 * This file is part of tmLQCD.
 *
 * tmLQCD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * tmLQCD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with tmLQCD.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File expo.c
 *
 *
 * The externally accessible functions are
 *
 *   su3 exposu3(su3adj p)
 *   su3 exposu3_check(su3adj p)
 *   su3 restoresu3(su3 u)
 *   Returns an element of su3
 *
 * Author: Martin Hasenbusch <martin.hasenbusch@desy.de>
 * Tue Aug 28 10:06:56 MEST 2001
 *
 ************************************************************************/

#ifdef HAVE_CONFIG_H
# include<config.h>
#endif
#ifdef SSE
# undef SSE
#endif
#ifdef SSE2
# undef SSE2
#endif
#ifdef SSE3
# undef SSE3
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "sse.h"
#include "su3.h"
#include "su3adj.h"
#include "expo.h"

su3 exposu3(su3adj p)
{
  int i;
  static su3 v,v2,vr;
  static double fac,r;
  static double a,b;
  static _Complex double a0,a1,a2,a1p;

  /* it writes 'p=vec(h_{j,mu})' in matrix form 'v' */  
  _make_su3(v,p);
  /* calculates v^2 */
  _su3_times_su3(v2,v,v);
  /* */
  a=0.5*(creal(v2.c00)+creal(v2.c11)+creal(v2.c22));
  /* 1/3 imaginary part of tr v*v2 */
  b = 0.33333333333333333*
    ( creal(v.c00)*cimag(v2.c00)+cimag(v.c00)*creal(v2.c00)
     +creal(v.c01)*cimag(v2.c10)+cimag(v.c01)*creal(v2.c10)
     +creal(v.c02)*cimag(v2.c20)+cimag(v.c02)*creal(v2.c20)
     +creal(v.c10)*cimag(v2.c01)+cimag(v.c10)*creal(v2.c01)
     +creal(v.c11)*cimag(v2.c11)+cimag(v.c11)*creal(v2.c11)
     +creal(v.c12)*cimag(v2.c21)+cimag(v.c12)*creal(v2.c21)
     +creal(v.c20)*cimag(v2.c02)+cimag(v.c20)*creal(v2.c02)
     +creal(v.c21)*cimag(v2.c12)+cimag(v.c21)*creal(v2.c12)
     +creal(v.c22)*cimag(v2.c22)+cimag(v.c22)*creal(v2.c22));
  a0 = (0.16059043836821615e-9) + cimag(a0) * I;    /*  1/13! */
  a0 = creal(a0);
  a1 = (0.11470745597729725e-10) + cimag(a1) * I;   /*  1/14! */
  a1 = creal(a1);
  a2 = (0.76471637318198165e-12) + cimag(a2) * I;   /*  1/15! */
  a2 = creal(a2);
  fac=0.20876756987868099e-8;      /*  1/12! */
  r=12.0;
  for(i = 3; i <= 15; i++) {
    a1p = (creal(a0) + a * creal(a2)) + cimag(a1p) * I;
    a1p = creal(a1p) + (cimag(a0) + a * cimag(a2)) * I;
    a0 = (fac - b * cimag(a2)) + cimag(a0) * I;
    a0 = creal(a0) + (+ b * creal(a2)) * I;
    a2 = a1;
    a1 = a1p;
    fac *= r;  
    r -= 1.0;
  }
  /* vr = a0 + a1*v + a2*v2 */
  vr.c00 = a0 + a1 * v.c00 + a2 * v2.c00;
  vr.c01 =      a1 * v.c01 + a2 * v2.c01;
  vr.c02 =      a1 * v.c02 + a2 * v2.c02;
  vr.c10 =      a1 * v.c10 + a2 * v2.c10;
  vr.c11 = a0 + a1 * v.c11 + a2 * v2.c11;
  vr.c12 =      a1 * v.c12 + a2 * v2.c12;
  vr.c20 =      a1 * v.c20 + a2 * v2.c20;
  vr.c21 =      a1 * v.c21 + a2 * v2.c21;
  vr.c22 = a0 + a1 * v.c22 + a2 * v2.c22;
  return vr;
}

su3 exposu3_check(su3adj p, int im) {
  /* compute the result by taylor series */
  static su3 v,v2,v3,vr;
  static double fac;
  int i;
  _make_su3(v, p);
  _su3_one(vr);
  _su3_acc(vr, v); 
  _su3_times_su3(v2, v, v);
  _su3_refac_acc(vr, 0.5, v2);
  fac = 0.5;
  for(i = 3; i <= im; i++) {
    fac = fac/i;
    _su3_times_su3(v3, v2, v);
    _su3_refac_acc(vr, fac, v3); 
    v2 = v3; 
  }
  return vr;
}


su3 restoresu3(su3 u)
{
  static su3 vr;
  static double n1,n2;
  
  /* normalize rows 1 and 2 */
  n1= creal(u.c00) * creal(u.c00) + cimag(u.c00) * cimag(u.c00)
    + creal(u.c01) * creal(u.c01) + cimag(u.c01) * cimag(u.c01)
    + creal(u.c02) * creal(u.c02) + cimag(u.c02) * cimag(u.c02);
  n1 = 1.0/sqrt(n1);
  n2= creal(u.c10) * creal(u.c10) + cimag(u.c10) * cimag(u.c10)
    + creal(u.c11) * creal(u.c11) + cimag(u.c11) * cimag(u.c11)
    + creal(u.c12) * creal(u.c12) + cimag(u.c12) * cimag(u.c12);
  n2= 1.0/sqrt(n2);
  
  vr.c00 = n1*u.c00;
  vr.c01 = n1*u.c01;
  vr.c02 = n1*u.c02;
  
  vr.c10 = n1*u.c10;
  vr.c11 = n1*u.c11;
  vr.c12 = n1*u.c12;
  
  /* compute  row 3 as the conjugate of the cross-product of 1 and 2 */ 
  vr.c20 = conj(vr.c01 * vr.c12 - vr.c02 * vr.c11);
  vr.c21 = conj(vr.c02 * vr.c10 - vr.c00 * vr.c11);
  vr.c22 = conj(vr.c00 * vr.c11 - vr.c01 * vr.c10);

  return vr;
}

/* Exponentiates a hermitian 3x3 matrix Q */
/* Convenience function -- wrapper around Hasenbusch's implementation */
void exposu3_in_place(su3 *u)
{
  static su3adj p;
  _trace_lambda(p, *u); /* Projects onto the Gell-Mann matrices */
  /* -2.0 to get su3 to su3adjoint consistency ****/
  p.d1 *= -0.5; p.d2 *= -0.5; p.d3 *= -0.5; p.d4 *= -0.5;
  p.d5 *= -0.5; p.d6 *= -0.5; p.d7 *= -0.5; p.d8 *= -0.5;
  *u = exposu3(p);
}
