// ***********************************************************************************************
// Idefix MHD astrophysical code
// Copyright(C) 2020 Geoffroy R. J. Lesur <geoffroy.lesur@univ-grenoble-alpes.fr
// and other code contributors
// Licensed under CeCILL 2.1 License, see COPYING for more information
// ***********************************************************************************************

#ifndef HYDRO_HDSOLVERS_HLLCHD_HPP_
#define HYDRO_HDSOLVERS_HLLCHD_HPP_

#include "../idefix.hpp"

// Compute Riemann fluxes from states using HLLC solver
template<const int DIR, const int Xn, const int Xt, const int Xb>
void Hydro::HllcHD() {
  idfx::pushRegion("Hydro::HLLC_Solver");
  
  int ioffset,joffset,koffset;
  ioffset=joffset=koffset=0;
  // Determine the offset along which we do the extrapolation
  switch(DIR) {
    case(IDIR):
      ioffset = 1;
      break;
    case(JDIR):
      joffset=1;
      break;
    case(KDIR):
      koffset=1;
      break;
    default:
      IDEFIX_ERROR("Wrong direction");
  }

  IdefixArray4D<real> PrimL = this->PrimL;
  IdefixArray4D<real> PrimR = this->PrimR;
  IdefixArray4D<real> Flux = this->FluxRiemann;
  IdefixArray3D<real> cMax = this->cMax;

  real gamma_m1 = this->gamma - ONE_F;
  real C2Iso = this->C2Iso;

  idefix_for("HLLC_Kernel",
             data->beg[KDIR],data->end[KDIR]+koffset,
             data->beg[JDIR],data->end[JDIR]+joffset,
             data->beg[IDIR],data->end[IDIR]+ioffset,
    KOKKOS_LAMBDA (int k, int j, int i) {
      // Primitive variables
      real vL[NVAR];
      real vR[NVAR];

      // Conservative variables
      real uL[NVAR];
      real uR[NVAR];

      // Flux (left and right)
      real fluxL[NVAR];
      real fluxR[NVAR];

      // Signal speeds
      real cL, cR, cmax;

      // 1-- Store the primitive variables on the left, right, and averaged states
      #pragma unroll
      for(int nv = 0 ; nv < NVAR; nv++) {
        vL[nv] = PrimL(nv,k,j,i);
        vR[nv] = PrimR(nv,k,j,i);
      }

      // 2-- Get the wave speed
#if HAVE_ENERGY
      cL = SQRT( gamma *(vL[PRS]/vL[RHO]));
      cR = SQRT( gamma *(vR[PRS]/vR[RHO]));
#else
      cL = SQRT(C2Iso);
      cR = cL;
#endif

      real cminL = vL[Xn] - cL;
      real cmaxL = vL[Xn] + cL;
      
      real cminR = vR[Xn] - cR;
      real cmaxR = vR[Xn] + cR;
      
      real SL = FMIN(cminL, cminR);
      real SR = FMAX(cmaxL, cmaxR);
      
      cmax  = FMAX(FABS(SL), FABS(SR));
      
      // 3-- Compute the conservative variables
      K_PrimToCons(uL, vL, gamma_m1);
      K_PrimToCons(uR, vR, gamma_m1);

      // 4-- Compute the left and right fluxes
      K_Flux(fluxL, vL, uL, C2Iso, Xn);
      K_Flux(fluxR, vR, uR, C2Iso, Xn);

      // 5-- Compute the flux from the left and right states
      if (SL > 0) {
        #pragma unroll
        for (int nv = 0 ; nv < NFLX; nv++) {
          Flux(nv,k,j,i) = fluxL[nv];
        }
      } else if (SR < 0) {
        #pragma unroll
        for (int nv = 0 ; nv < NFLX; nv++) {
          Flux(nv,k,j,i) = fluxR[nv];
        }
      } else {
        real usL[NVAR];
        real usR[NVAR];
        real vs;
          
#if HAVE_ENERGY
        real qL, qR, wL, wR;
        qL = vL[PRS] + uL[Xn]*(vL[Xn] - SL);
        qR = vR[PRS] + uR[Xn]*(vR[Xn] - SR);

        wL = vL[RHO]*(vL[Xn] - SL);
        wR = vR[RHO]*(vR[Xn] - SR);

        vs = (qR - qL)/(wR - wL); // wR - wL > 0 since SL < 0, SR > 0

        usL[RHO] = uL[RHO]*(SL - vL[Xn])/(SL - vs);
        usR[RHO] = uR[RHO]*(SR - vR[Xn])/(SR - vs);
        EXPAND(usL[Xn] = usL[RHO]*vs;     usR[Xn] = usR[RHO]*vs;      ,
                usL[Xt] = usL[RHO]*vL[Xt]; usR[Xt] = usR[RHO]*vR[Xt];  ,
                usL[Xb] = usL[RHO]*vL[Xb]; usR[Xb] = usR[RHO]*vR[Xb];)

        usL[ENG] =    uL[ENG]/vL[RHO]
                    + (vs - vL[Xn])*(vs + vL[PRS]/(vL[RHO]*(SL - vL[Xn])));
        usR[ENG] =    uR[ENG]/vR[RHO]
                    + (vs - vR[Xn])*(vs + vR[PRS]/(vR[RHO]*(SR - vR[Xn])));

        usL[ENG] *= usL[RHO];
        usR[ENG] *= usR[RHO];
#else
        real scrh = 1.0/(SR - SL);
        real rho  = (SR*uR[RHO] - SL*uL[RHO] - fluxR[RHO] + fluxL[RHO])*scrh;
        real mx   = (SR*uR[Xn] - SL*uL[Xn] - fluxR[Xn] + fluxL[Xn])*scrh;

        usL[RHO] = usR[RHO] = rho;
        usL[Xn] = usR[Xn] = mx;
        vs  = (  SR*fluxL[RHO] - SL*fluxR[RHO]
                + SR*SL*(uR[RHO] - uL[RHO]));
        vs *= scrh;
        vs /= rho;
        EXPAND(                                            ,
                usL[Xt] = rho*vL[Xt]; usR[Xt] = rho*vR[Xt]; ,
                usL[Xb] = rho*vL[Xb]; usR[Xb] = rho*vR[Xb];)
#endif
        
    // Compute the flux from the left and right states
        if (vs >= 0.0) {
          #pragma unroll
          for(int nv = 0 ; nv < NVAR; nv++) {
            Flux(nv,k,j,i) = fluxL[nv] + SL*(usL[nv] - uL[nv]);
          }
        } else {
          #pragma unroll
          for(int nv = 0 ; nv < NVAR; nv++) {
            Flux(nv,k,j,i) = fluxR[nv] + SR*(usR[nv] - uR[nv]);
          }
        }
      }

      //6-- Compute maximum wave speed for this sweep
      cMax(k,j,i) = cmax;
  });

  idfx::popRegion();
}

#endif  // HYDRO_HDSOLVERS_HLLCHD_HPP_