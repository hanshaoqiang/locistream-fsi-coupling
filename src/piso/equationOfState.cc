//-----------------------------------------------------------------------------
// Description: This file contains rules for the equation-of-state.
//
// Authors: Siddharth Thakur and Jeff Wright
//          Streamline Numerics Inc.
//          Gainesville, Florida 32609
//-----------------------------------------------------------------------------

// Standard library includes.
#include<vector>
using std::vector ;

// Loci includes.
#include <Loci.h>
#include <Tools/fpe.h>

// Fluid physcis library includes.
#include "eos.h"
using fluidPhysics::EOS ;

// StreamUns includes.
#include "sciTypes.h"

namespace streamUns {

  //---------------------------------------------------------------------------
  // Rules for computing the EOS on the boundaries.

  // Solves for the equation of state on boundary faces.
  class GetEOSStateBoundaryYPT : public pointwise_rule {
    private:
      const_Map ci ;
      const_param<EOS> eos ;
      const_store<real> p_f,temperature_f ;
      const_storeVec<real> y_f ;
      const_storeVec<float> hint ;
      store<EOS::State> eosState_f ;
      storeVec<real> eosMixtureState_f ;
    public:

      // Define input and output.
      GetEOSStateBoundaryYPT() {
        name_store("ci",ci) ;
        name_store("eos",eos) ;
        name_store("p_f",p_f) ;
        name_store("temperature_f",temperature_f) ;
        name_store("y_f",y_f) ;
        name_store("hint_n",hint) ;
        name_store("eos_state_f",eosState_f) ;
        name_store("eos_mixture_state_f",eosMixtureState_f) ;
        input("eos,p_f,temperature_f,y_f,ci->hint_n") ;
        output("eos_state_f,eos_mixture_state_f") ;
      }

      // Get the EOS state for a single face.
      void calculate(Entity face) {
        eosState_f[face]=eos->State_from_mixture_p_T(y_f[face],p_f[face],
          temperature_f[face],eosMixtureState_f[face],hint[ci[face]]) ;
      }

      // Calculate density for all faces in sequence.
      virtual void compute(const sequence &seq) {
        eosMixtureState_f.setVecSize(eos->mixtureStateSize()) ;
        do_loop(seq,this) ;
      }

  } ;

  register_rule<GetEOSStateBoundaryYPT> registerGetEOSStateBoundaryYPT ;
 
  //---------------------------------------------------------------------------
  // Rules for marching the equation of state.

  // Time build rule for the equation of state.
  class TimeBuildEOSState : public pointwise_rule {
    private:
      store<EOS::State> eosState ;
    public:

      // Define input and output.
      TimeBuildEOSState() {
        name_store("eos_state_ic",eosState) ;
        input("eos_state_ic") ;
        output("eos_state{n=0}=eos_state_ic") ;
        constraint("geom_cells,compressibleFlow") ;
      }

      // Empty compute rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<TimeBuildEOSState> registerTimeBuildEOSState ;

  // Iteration build rule for the equation of state.
  class IterationBuildEOSState : public pointwise_rule {
    private:
      store<EOS::State> eosState ;
    public:

      // Define input and output.
      IterationBuildEOSState() {
        name_store("eos_state_star{n}",eosState) ;
        input("eos_state_star{n}") ;
        output("eos_state{n,it=0}=eos_state_star{n}") ;
        constraint("geom_cells{n},compressibleFlow{n}") ;
      }

      // Empty compute for rename rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<IterationBuildEOSState> registerIterationBuildEOSState ;

  // Iteration advance rule for the equation of state.
  class IterationAdvanceEOSState : public pointwise_rule {
    private:
      const_param<bool> iterationFinished ;
      store<EOS::State> eosState ;
    public:

      // Define input and output.
      IterationAdvanceEOSState() {
        name_store("eos_state_corrected{n,it}",eosState) ;
        input("eos_state_corrected{n,it}") ;
        output("eos_state{n,it+1}=eos_state_corrected{n,it}") ;
        constraint("geom_cells{n,it},compressibleFlow{n,it}") ;
      }

      // Empty compute for rename rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<IterationAdvanceEOSState> registerIterationAdvanceEOSState ;

  // Iteration collapse rule for the equation of state.
  class IterationCollapseEOSState : public pointwise_rule {
    private:
      const_store<EOS::State> eosStateOld ;
      store<EOS::State> eosState ;
    public:

      // Define input and output.
      IterationCollapseEOSState() {
        name_store("eos_state{n,it}",eosState) ;
        input("eos_state{n,it}") ;
        output("eos_state{n+1}=eos_state{n,it}") ;
        conditional("iterationFinished{n,it-1}") ;
        constraint("geom_cells{n,it},compressibleFlow{n,it}") ;
      }

      // Empty compute for rename rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<IterationCollapseEOSState> registerIterationCollapseEOSState ;
 
  //---------------------------------------------------------------------------
  // Rules for marching the mixuture state which contains information about
  // the state of each species.

  // Time build rule for the mixture state.
  class TimeBuildEOSMixtureState : public pointwise_rule {
    private:
      storeVec<real> eosMixtureState ;
    public:

      // Define input and output.
      TimeBuildEOSMixtureState() {
        name_store("eos_mixture_state_ic",eosMixtureState) ;
        input("eos_mixture_state_ic") ;
        output("eos_mixture_state{n=0}=eos_mixture_state_ic") ;
        constraint("geom_cells,compressibleFlow") ;
      }

      // Empty compute rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<TimeBuildEOSMixtureState> registerTimeBuildEOSMixtureState ;

  // Iteration build rule for the mixture state.
  class IterationBuildEOSMixtureState : public pointwise_rule {
    private:
      storeVec<real> eosMixtureState ;
    public:

      // Define input and output.
      IterationBuildEOSMixtureState() {
        name_store("eos_mixture_state_star{n}",eosMixtureState) ;
        input("eos_mixture_state_star{n}") ;
        output("eos_mixture_state{n,it=0}=eos_mixture_state_star{n}") ;
        constraint("geom_cells{n},compressibleFlow{n}") ;
      }

      // Empty compute for rename rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<IterationBuildEOSMixtureState>
    registerIterationBuildEOSMixtureState ;

  // Iteration advance rule for the mixture state.
  class IterationAdvanceEOSMixtureState : public pointwise_rule {
    private:
      const_param<bool> iterationFinished ;
      storeVec<real> eosMixtureState ;
    public:

      // Define input and output.
      IterationAdvanceEOSMixtureState() {
        name_store("eos_mixture_state_corrected{n,it}",eosMixtureState) ;
        input("eos_mixture_state_corrected{n,it}") ;
        output("eos_mixture_state{n,it+1}=eos_mixture_state_corrected{n,it}") ;
        constraint("geom_cells{n,it},compressibleFlow{n,it}") ;
      }

      // Empty compute for rename rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<IterationAdvanceEOSMixtureState>
    registerIterationAdvanceEOSMixtureState ;

  // Iteration collapse rule for the equation of state.
  class IterationCollapseEOSMixtureState : public pointwise_rule {
    private:
      storeVec<real> eosMixtureState ;
    public:

      // Define input and output.
      IterationCollapseEOSMixtureState() {
        name_store("eos_mixture_state{n,it}",eosMixtureState) ;
        input("eos_mixture_state{n,it}") ;
        output("eos_mixture_state{n+1}=eos_mixture_state{n,it}") ;
        conditional("iterationFinished{n,it-1}") ;
        constraint("geom_cells{n,it},compressibleFlow{n,it}") ;
      }

      // Empty compute for rename rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<IterationCollapseEOSMixtureState>
    registerIterationCollapseEOSMixtureState ;
 
  //---------------------------------------------------------------------------
  // Rules for marching the EOS inversion hints.

  // Time build rule for the hint.
  class TimeBuildHint : public pointwise_rule {
    private:
      storeVec<float> hint_n_ic ;
    public:

      // Define input and output.
      TimeBuildHint() {
        name_store("hint_n_ic",hint_n_ic) ;
        input("hint_n_ic") ;
        output("hint_n{n=0}=hint_n_ic") ;
        constraint("geom_cells,compressibleFlow") ;
      }

      // Empty compute rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<TimeBuildHint> registerTimeBuildHint ;

  // Iteration build rule for the hint.
  class IterationBuildHint : public pointwise_rule {
    private:
      storeVec<float> hint ;
    public:

      // Define input and output.
      IterationBuildHint() {
        name_store("hint_star{n}",hint) ;
        input("hint_star{n}") ;
        output("hint_n{n,it=0}=hint_star{n}") ;
        constraint("geom_cells{n},compressibleFlow{n}") ;
      }

      // Empty compute for rename rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<IterationBuildHint> registerIterationBuildHint ;

  // Iteration advance rule for the hint.
  class IterationAdvanceHint : public pointwise_rule {
    private:
      const_param<bool> iterationFinished ;
      storeVec<float> hint ;
    public:

      // Define input and output.
      IterationAdvanceHint() {
        name_store("hint_corrected{n,it}",hint) ;
        input("hint_corrected{n,it}") ;
        output("hint_n{n,it+1}=hint_corrected{n,it}") ;
        constraint("geom_cells{n,it},compressibleFlow{n,it}") ;
      }

      // Empty compute for rename rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<IterationAdvanceHint> registerIterationAdvanceHint ;

  // Iteration collapse rule for the hint.
  class IterationCollapseHint : public pointwise_rule {
    private:
      storeVec<float> hint ;
    public:

      // Define input and output.
      IterationCollapseHint() {
        name_store("hint_n{n,it}",hint) ;
        input("hint_n{n,it}") ;
        output("hint_n{n+1}=hint_n{n,it}") ;
        conditional("iterationFinished{n,it-1}") ;
        constraint("geom_cells{n,it},compressibleFlow{n,it}") ;
      }

      // Empty compute for rename rule.
      void compute(const sequence &seq) {}
  } ;

  register_rule<IterationCollapseHint> registerIterationCollapseHint ;

  // Rule to solver for the new thermodynamic state from the pressure,
  // static enthalpy and species mass fractions.
  class GetDensityTemperatureHintPredictor : public pointwise_rule {
    private:
      const_param<EOS> eos ;
      const_store<vect3d> v ;
      const_store<real> p,h ;
      const_storeVec<real> y ;
      store<real> rho,temperature ;
      store<EOS::State> eos_state ;
      storeVec<real> eos_mixture_state ;
      storeVec<float> hint ;
    public:

      // Define input and output. NOTE: Had to include eos_mixture_state{n}
      // as an input, otherwise get strange Loci errors.
      GetDensityTemperatureHintPredictor() {
        name_store("eos{n}",eos) ;
        name_store("vCorrected_p{n}",v) ;
        name_store("pCorrected_p{n}",p) ;
        name_store("h{n}",h) ;
        name_store("y{n}",y) ;
        name_store("rhoStar{n}",rho) ;
        name_store("temperatureStar{n}",temperature) ;
        name_store("eos_state{n}",eos_state) ;
        name_store("eos_mixture_state{n}",eos_mixture_state) ;
        name_store("hint_n{n}",hint) ;
        input("eos{n},vCorrected_p{n},pCorrected_p{n}") ;
        input("h{n},y{n},eos_mixture_state{n}") ;
        output("rhoStar{n},temperatureStar{n}") ;
        output("eos_state_star{n}=eos_state{n}") ;
        output("eos_mixture_state_star{n}=eos_mixture_state{n}") ;
        output("hint_star{n}=hint_n{n}") ;
        constraint("geom_cells{n},compressibleFlow{n}") ;
      }

      // Compute density and temperature for a single entity.
      void calculate(Entity cell) {
        real staticEnthalpy=h[cell]-0.5*dot(v[cell],v[cell]) ;
        eos_state[cell]=eos->State_from_p_h(y[cell],p[cell],staticEnthalpy,
          eos_mixture_state[cell],hint[cell]) ;
        rho[cell]=eos_state[cell].density() ;
        temperature[cell]=eos_state[cell].temperature() ;
        eos->getHint(hint[cell],eos_state[cell],eos_mixture_state[cell]) ;
      }

      // Loop over entities.
      virtual void compute(const sequence &seq) { do_loop(seq,this) ; }
  } ;

  register_rule<GetDensityTemperatureHintPredictor>
    registerGetDensityTemperatureHintPredictor ;

  // Rule to solver for the new thermodynamic state from the pressure,
  // static enthalpy and species mass fractions.
  class GetDensityTemperatureHintCorrector : public pointwise_rule {
    private:
      const_param<EOS> eos ;
      const_store<vect3d> v ;
      const_store<real> p,h ;
      const_storeVec<real> y ;
      store<real> rho,temperature ;
      store<EOS::State> eos_state ;
      storeVec<real> eos_mixture_state ;
      storeVec<float> hint ;
    public:

      // Define input and output.
      GetDensityTemperatureHintCorrector() {
        name_store("eos{n,it}",eos) ;
        name_store("vCorrected_c{n,it}",v) ;
        name_store("pCorrected_c{n,it}",p) ;
        name_store("h{n,it}",h) ;
        name_store("y{n,it}",y) ;
        name_store("rhoCorrected{n,it}",rho) ;
        name_store("temperatureCorrected{n,it}",temperature) ;
        name_store("eos_state{n,it}",eos_state) ;
        name_store("eos_mixture_state{n,it}",eos_mixture_state) ;
        name_store("hint_n{n,it}",hint) ;
        input("eos{n,it},vCorrected_c{n,it}") ;
        input("pCorrected_c{n,it},h{n,it},y{n,it}") ;
        output("rhoCorrected{n,it},temperatureCorrected{n,it}") ;
        output("eos_state_corrected{n,it}=eos_state{n,it}") ;
        output("eos_mixture_state_corrected{n,it}=eos_mixture_state{n,it}") ;
        output("hint_corrected{n,it}=hint_n{n,it}") ;
        constraint("geom_cells{n,it},compressibleFlow{n,it}") ;
      }

      // Compute density and temperature for a single entity.
      void calculate(Entity cell) {
        real staticEnthalpy=h[cell]-0.5*dot(v[cell],v[cell]) ;
        eos_state[cell]=eos->State_from_p_h(y[cell],p[cell],staticEnthalpy,
          eos_mixture_state[cell],hint[cell]) ;
        rho[cell]=eos_state[cell].density() ;
        temperature[cell]=eos_state[cell].temperature() ;
        eos->getHint(hint[cell],eos_state[cell],eos_mixture_state[cell]) ;
      }

      // Loop over entities.
      virtual void compute(const sequence &seq) { do_loop(seq,this) ; }
  } ;

  register_rule<GetDensityTemperatureHintCorrector>
    registerGetDensityTemperatureHintCorrector ;
}
