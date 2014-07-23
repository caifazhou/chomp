/*
* Copyright (c) 2008-2014, Matt Zucker
*
* This file is provided under the following "BSD-style" License:
*
* Redistribution and use in source and binary forms, with or
* without modification, are permitted provided that the following
* conditions are met:
*
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
* USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef _CHOMP_OPTIMIZER_H_
#define _CHOMP_OPTIMIZER_H_

#include "chomputil.h"

#include <iostream>
#include <vector>
#include <pthread.h>
#include "../mzcommon/TimeUtil.h"
#include "chomp_gradient.h"

namespace chomp {

  class ConstraintFactory;
  class Constraint;
  class HMC;
  class Chomp;


  class ChompOptimizer {
  public:

    ConstraintFactory* factory;        

    ChompObserver* observer;

    ChompGradient* gradient;

    ChompObjectiveType objective_type;
    
    int M; // degrees of freedom

    // the actual desired number of timesteps
    int maxN; 

    // the base (minimum) number of timesteps
    int minN;

    int N; // number of timesteps
    int N_sub; // number of timesteps for subsampled trajectory
    
    // current trajectory of size N-by-M
    // Stored in RowMajor format
    MatXR xi;
    SubMatMapR xi_sub; // current trajectory of size N_sub-by-M

    MatX h; // constraint function of size k-by-1
    MatX h_sub; // constraint function of size k_sub-by-1

    MatX H; // constraint Jacobian of size k-by-M*N
    MatX H_sub; // constraint Jacobian of size k_sub-by-1

    double hmag; // inf. norm magnitude of constraint violation

    // working variables
    MatX H_trans, P, P_trans, HP, Y, W, g_trans, delta, delta_trans; 

    double alpha;
    double objRelErrTol;
    // last_objective : save the last objective, for use with rejection.
    double lastObjective;
    

    size_t cur_iter
    size_t min_global_iter, max_global_iter;
    size_t min_local_iter, max_local_iter;

    bool full_global_at_final;

    double t_total; // total time for (N+1) timesteps
    double dt; // computed automatically from t_total and N
    double inv_dt; // computed automatically from t_total and N
    
    double timeout_seconds;
    bool canTimeout, didTimeout;
    TimeStamp stop_time;

    pthread_mutex_t trajectory_mutex;
    bool use_mutex;

    Eigen::LDLT<MatX> cholSolver;

    std::vector<Constraint*> constraints; // vector of size N
    
    //used for goal set chomp.
    Constraint * goalset;
    bool use_goalset;
     
    bool use_momentum;
    MatX momentum;

    HMC * hmc;

    ChompOptimizer(ConstraintFactory* f,
          const MatX& xi_init, // should be N-by-M
          int nmax,
          double al = 0.1,
          double obstol = 0.01,
          size_t max_global_iter=size_t(-1),
          size_t max_local_iter=size_t(-1),
          double t_total=1.0,
          double timeout_seconds=-1.0,
          bool use_momentum = false);
    
    ~Chomp(){
        if( use_mutex ){
            pthread_mutex_destroy( &trajectory_mutex );
        }
    }
    void lockTrajectory(){
        if (use_mutex){
            pthread_mutex_lock( &trajectory_mutex );
        }
    }
    void unlockTrajectory(){
        if (use_mutex){
            pthread_mutex_unlock( &trajectory_mutex );
        }
    }

    template <class Derived>
    void updateTrajectory( const Eigen::MatrixBase<Derived1> & delta,
                       bool subsample )

    void initMutex();
   
    void updateGradient();

    void clearConstraints();

    //prepares chomp to be run at a resolution level
    void prepareChomp();    

    // precondition: prepareChomp was called for this resolution level
    void prepareChompIter();

    // precondition: prepareChomp was called for this resolution level
    // updates chomp equation until convergence at the current level
    void runChomp(bool global, bool local);

    // calls runChomp and upsamples until N big enough
    // precondition: N <= maxN
    // postcondition: N >= maxN
    void solve(bool doGlobalSmoothing, bool doLocalSmoothing);

    // get the tick, respecting endpoint repetition
    MatX getTickBorderRepeat(int tick) const;

    // upsamples the trajectory by 2x
    void upsample();

    // single iteration of chomp
    void chompGlobal();
    
    // single iteration of local smoothing
    //
    // precondition: prepareChompIter has been called since the last
    // time xi was modified
    void localSmooth();

    // upsamples trajectory, projecting onto constraint for each new
    // trajectory element.
    void constrainedUpsampleTo(int Nmax, double htol, double hstep=0.5);

    // returns true if performance has converged
    bool goodEnough(double oldObjective, double newObjective);

    // call the observer if there is one
    int notify(ChompEventType event,
               size_t iter,
               double curObjective, 
               double lastObjective,
               double constraintViolation) const;

    //Give a goal set in the form of a constraint for chomp to use on the
    //  first resolution level.
    void useGoalSet( Constraint * goalset );

    //call after running goal set chomp.
    void finishGoalSet(); 
  };




}


#endif
