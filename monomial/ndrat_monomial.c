/***********************************************************************
 *
 * Copyright (C) 2008 Thomas Chiarappa, Carsten Urbach
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
 ***********************************************************************/

#ifdef HAVE_CONFIG_H
# include<config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "global.h"
#include "su3.h"
#include "linalg_eo.h"
#include "start.h"
#include "gettime.h"
#include "solver/solver.h"
#include "deriv_Sb.h"
#include "init/init_chi_spinor_field.h"
#include "operator/tm_operators.h"
#include "operator/tm_operators_nd.h"
#include "operator/Hopping_Matrix.h"
#include "monomial/monomial.h"
#include "hamiltonian_field.h"
#include "boundary.h"
#include "operator/clovertm_operators.h"
#include "operator/clover_leaf.h"
#include "rational/rational.h"
#include "phmc.h"
#include "ndrat_monomial.h"

static void nd_set_global_parameter(monomial * const mnl) {

  g_mubar = mnl->mubar;
  g_epsbar = mnl->epsbar;
  g_kappa = mnl->kappa;
  g_c_sw = mnl->c_sw;
  boundary(g_kappa);
  phmc_cheb_evmin = mnl->EVMin;
  phmc_invmaxev = mnl->EVMaxInv;
  phmc_cheb_evmax = 1.;

  return;
}


/********************************************
 *
 * Here \delta S_b is computed
 *
 ********************************************/

void ndrat_derivative(const int id, hamiltonian_field_t * const hf) {
  int j, k;
  monomial * mnl = &monomial_list[id];
  double atime, etime;
  atime = gettime();
  nd_set_global_parameter(mnl);
  if(mnl->type == NDCLOVERRAT) {
    for(int i = 0; i < VOLUME; i++) { 
      for(int mu = 0; mu < 4; mu++) { 
	_su3_zero(swm[i][mu]);
	_su3_zero(swp[i][mu]);
      }
    }
  
    // we compute the clover term (1 + T_ee(oo)) for all sites x
    sw_term( (const su3**) hf->gaugefield, mnl->kappa, mnl->c_sw); 
    // we invert it for the even sites only
    sw_invert_nd(mnl->mubar*mnl->mubar - mnl->epsbar*mnl->epsbar);
  }
  mnl->forcefactor = -mnl->EVMaxInv;

  /* Recall:  The GAMMA_5 left of  delta M_eo  is done in  deriv_Sb !!! */

  /* Here comes the definitions for the chi_j fields */
  /* from  j=0  (chi_0 = phi)  .....  to j = n-1 */
  /* in  g_chi_up_spinor_field[0] (g_chi_dn_spinor_field[0] we expect */
  /* to find the phi field, the pseudo fermion field                  */
  /* i.e. must be equal to mnl->pf (mnl->pf2)                         */
  
  assign(g_chi_up_spinor_field[0], mnl->pf, VOLUME/2);
  assign(g_chi_dn_spinor_field[0], mnl->pf2, VOLUME/2);
  
  for(k = 1; k < (mnl->MDPolyDegree-1); k++) {
    Qsw_tau1_sub_const_ndpsi(g_chi_up_spinor_field[k], g_chi_dn_spinor_field[k], 
			   g_chi_up_spinor_field[k-1], g_chi_dn_spinor_field[k-1], 
			   mnl->MDPolyRoots[k-1]);
  }
  
  /* Here comes the remaining fields  chi_k ; k=n,...,2n-1  */
  /*They are evaluated step-by-step overwriting the same field (mnl->MDPolyDegree)*/
  
  assign(g_chi_up_spinor_field[mnl->MDPolyDegree], g_chi_up_spinor_field[mnl->MDPolyDegree-2], VOLUME/2);
  assign(g_chi_dn_spinor_field[mnl->MDPolyDegree], g_chi_dn_spinor_field[mnl->MDPolyDegree-2], VOLUME/2);
  
  for(j = (mnl->MDPolyDegree-1); j > 0; j--) {
    assign(g_chi_up_spinor_field[mnl->MDPolyDegree-1], g_chi_up_spinor_field[mnl->MDPolyDegree], VOLUME/2);
    assign(g_chi_dn_spinor_field[mnl->MDPolyDegree-1], g_chi_dn_spinor_field[mnl->MDPolyDegree], VOLUME/2);
    
    Qsw_tau1_sub_const_ndpsi(g_chi_up_spinor_field[mnl->MDPolyDegree], g_chi_dn_spinor_field[mnl->MDPolyDegree], 
			     g_chi_up_spinor_field[mnl->MDPolyDegree-1], g_chi_dn_spinor_field[mnl->MDPolyDegree-1], 
			     mnl->MDPolyRoots[2*mnl->MDPolyDegree-3-j]);
    
    /* Get the even parts of the  (j-1)th  chi_spinors */
    H_eo_sw_ndpsi(mnl->w_fields[0], mnl->w_fields[1], 
		  g_chi_up_spinor_field[j-1], g_chi_dn_spinor_field[j-1]);
    
    /* \delta M_eo sandwitched by  chi[j-1]_e^\dagger  and  chi[2N-j]_o */
    deriv_Sb(EO, mnl->w_fields[0], g_chi_up_spinor_field[mnl->MDPolyDegree], hf, mnl->forcefactor);/* UP */
    deriv_Sb(EO, mnl->w_fields[1], g_chi_dn_spinor_field[mnl->MDPolyDegree], hf, mnl->forcefactor);/* DN */

    /* Get the even parts of the  (2N-j)-th  chi_spinors */
    H_eo_sw_ndpsi(mnl->w_fields[2], mnl->w_fields[3], 
		  g_chi_up_spinor_field[mnl->MDPolyDegree], g_chi_dn_spinor_field[mnl->MDPolyDegree]);
    
    /* \delta M_oe sandwitched by  chi[j-1]_o^\dagger  and  chi[2N-j]_e */
    deriv_Sb(OE, g_chi_up_spinor_field[j-1], mnl->w_fields[2], hf, mnl->forcefactor);
    deriv_Sb(OE, g_chi_dn_spinor_field[j-1], mnl->w_fields[3], hf, mnl->forcefactor);

    // even/even sites sandwiched by gamma_5 Y_e and gamma_5 X_e
    sw_spinor(EE, mnl->w_fields[3], mnl->w_fields[0], mnl->forcefactor);
    // odd/odd sites sandwiched by gamma_5 Y_o and gamma_5 X_o
    sw_spinor(OO, g_chi_up_spinor_field[j-1], g_chi_dn_spinor_field[mnl->MDPolyDegree], mnl->forcefactor);

    // even/even sites sandwiched by gamma_5 Y_e and gamma_5 X_e
    sw_spinor(EE, mnl->w_fields[2], mnl->w_fields[1], mnl->forcefactor);
    // odd/odd sites sandwiched by gamma_5 Y_o and gamma_5 X_o
    sw_spinor(OO, g_chi_dn_spinor_field[j-1], g_chi_up_spinor_field[mnl->MDPolyDegree], mnl->forcefactor);
  }
  // trlog part does not depend on the normalisation of the polynomial
  sw_deriv_nd(EE);
  sw_all(hf, mnl->kappa, mnl->c_sw);
  etime = gettime();
  if(g_debug_level > 1 && g_proc_id == 0) {
    printf("# Time for %s monomial derivative: %e s\n", mnl->name, etime-atime);
  }
  return;
}


void ndrat_heatbath(const int id, hamiltonian_field_t * const hf) {
  monomial * mnl = &monomial_list[id];
  solver_pm_t solver_pm;
  spinor *up0, *dn0, *up1, *dn1, *dummy;
  double atime, etime;
  atime = gettime();
  nd_set_global_parameter(mnl);
  g_mu3 = 0.;
  if(mnl->type == NDCLOVERRAT) {
    init_sw_fields();
    sw_term((const su3**)hf->gaugefield, mnl->kappa, mnl->c_sw); 
    sw_invert_nd(mnl->mubar*mnl->mubar - mnl->epsbar*mnl->epsbar);
  }
  // we measure before the trajectory!
  if((mnl->rec_ev != 0) && (hf->traj_counter%mnl->rec_ev == 0)) {
    phmc_compute_ev(hf->traj_counter-1, id, &Qtm_pm_ndbipsi);
  }

  mnl->energy0 = 0.;
  random_spinor_field_eo(g_chi_up_spinor_field[0], mnl->rngrepro, RN_GAUSS);
  mnl->energy0 = square_norm(g_chi_up_spinor_field[0], VOLUME/2, 1);

  random_spinor_field_eo(g_chi_dn_spinor_field[0], mnl->rngrepro, RN_GAUSS);
  mnl->energy0 += square_norm(g_chi_dn_spinor_field[0], VOLUME/2, 1);
  solver_pm.max_iter = mnl->maxiter;
  solver_pm.eps_sq = mnl->accprec;
  solver_pm.no_shifts = mnl->rat.np;
  solver_pm.shifts = mnl->rat.nu;
  solver_pm.type = CGMMSND;
  solver_pm.g = &Qtm_pm_ndpsi;
  solver_pm.N = VOLUME/2;
  cg_mms_tm_nd(&g_chi_up_spinor_field[1], &g_chi_dn_spinor_field[1],
	       g_chi_up_spinor_field[0], g_chi_dn_spinor_field[0], 
	       &solver_pm);

  for(int j = 0; j < (mnl->rat.np); j++){

  }
  
  assign(mnl->pf, g_chi_up_spinor_field[0], VOLUME/2);
  assign(mnl->pf2, g_chi_dn_spinor_field[0], VOLUME/2);
  etime = gettime();
  if(g_proc_id == 0) {
    if(g_debug_level > 1) {
      printf("# Time for %s monomial heatbath: %e s\n", mnl->name, etime-atime);
    }
    if(g_debug_level > 3) {
      printf("called ndrat_heatbath for id %d energy %f\n", id, mnl->energy0);
    }
  }
  return;
}


double ndrat_acc(const int id, hamiltonian_field_t * const hf) {
  solver_pm_t solver_pm;
  monomial * mnl = &monomial_list[id];
  spinor *up0, *dn0, *up1, *dn1, *dummy;
  double atime, etime;
  atime = gettime();
  nd_set_global_parameter(mnl);
  g_mu3 = 0.;
  if(mnl->type == NDCLOVERRAT) {
    sw_term((const su3**) hf->gaugefield, mnl->kappa, mnl->c_sw); 
    sw_invert_nd(mnl->mubar*mnl->mubar - mnl->epsbar*mnl->epsbar);
  }
  mnl->energy1 = 0.;

  solver_pm.max_iter = mnl->maxiter;
  solver_pm.eps_sq = mnl->accprec;
  solver_pm.no_shifts = mnl->rat.np;
  solver_pm.shifts = mnl->rat.mu;
  solver_pm.type = CGMMSND;
  solver_pm.g = &Qtm_pm_ndpsi;
  solver_pm.N = VOLUME/2;
  cg_mms_tm_nd(g_chi_up_spinor_field, g_chi_dn_spinor_field,
	       mnl->pf, mnl->pf2,
	       &solver_pm);


  for(int j = 1; j <= (mnl->MDPolyDegree-1); j++) {

  }
  
  //mnl->energy1 = square_norm(up0, VOLUME/2, 1);
  //mnl->energy1 += square_norm(dn0, VOLUME/2, 1);
  etime = gettime();
  if(g_proc_id == 0) {
    if(g_debug_level > 1) {
      printf("# Time for %s monomial acc step: %e s\n", mnl->name, etime-atime);
    }
    if(g_debug_level > 3) {
      printf("called ndrat_acc for id %d dH = %1.10e\n", id, mnl->energy1 - mnl->energy0);
    }
  }
  return(mnl->energy1 - mnl->energy0);
}


int init_ndrat_monomial(const int id) {
  monomial * mnl = &monomial_list[id];  
  
  init_rational(&mnl->rat);

  mnl->EVMin = mnl->StildeMin / mnl->StildeMax;
  mnl->EVMax = 1.;
  mnl->EVMaxInv = 1./(sqrt(mnl->StildeMax));

  if(init_chi_spinor_field(VOLUMEPLUSRAND/2, (mnl->rat.np+1)) != 0) {
    fprintf(stderr, "Not enough memory for Chi fields! Aborting...\n");
    exit(0);
  }
  else {
    printf("Initialised %d chi fields\n",mnl->rat.np+1 );
  }


  return(0);
}

