
#include "MotionOptimizer.h"

namespace chomp {

//constructor.
MotionOptimizer::MotionOptimizer(ConstraintFactory * factory,
                                 ChompObserver * observer,
                                 double obstol,
                                 double timeout_seconds,
                                 size_t max_iter,
                                 const MatX & lower_bounds,
                                 const MatX & upper_bounds,
                                 OptimizationAlgorithm algorithm) :
    gradient( trajectory ),
    factory ( factory ),
    observer( observer ),
    obstol( obstol )
{
}

void MotionOptimizer::solve(){

    //optimize at the current resolution
    optimize();

    //if the current resolution is not the final resolution,
    //  upsample and then optimize. Repeat until the final resolution
    //  is reached or exceeded.
    while ( trajectory.rows() < N_max ){

        //upsample the trajectory and prepare for the next
        //  stage of optimization.
        trajectory.upsample();

        optimize();
    }
}


void MotionOptimizer::optimize( ){
    
    if ( use_goalset ){ prepareGoalSet(); }

    gradient.prepareRun( trajectory, use_goalset );

    // Subsample
    bool subsample = trajectory.N() > N_min && !use_goalset &&
                     !(full_global_at_final && trajectory.N() >= N_max);


    if( subsample ){ trajectory.subsample(); }


    //create the optimizer
    //TODO include other optimization schemes
    OptimizerBase * optimizer;

    switch ( algorithm ){
        case CHOMP:
            optimizer = new ChompOptimizer(trajectory, 
                                           factory, 
                                           &gradient, 
                                           observer, 
                                           obstol,
                                           timeout_seconds,
                                           max_iterations,
                                           lower_bounds,
                                           upper_bounds );
            break;

        case THE_OTHER:
#if NLOPT_FOUND
            //TODO add nlopt optimization schemes
            std::cerr << "NLopt optimization schemes are unimplemented"
                      << std::endl;
#else 
            std::cerr << "NLopt optimization libraries are"
                      << " not available, please use a different"
                      << " optimization algorithm" << std::endl;
            return; 
#endif
            return;
            break;

        default:
            std::cerr << "Unrecognized optimization scheme";
            return;
            break;
    }

    optimizer->solve();

    delete optimizer;

}

void MotionOptimizer::setGoalset( Constraint * goalset ){
      this->goalset = goalset;
      use_goalset = true;
}

void MotionOptimizer::prepareGoalSet(){
    
    //do not subsample if doing goalset run.
    N_sub = 0;
    
    trajectory.startGoalSet();
    
    //add the goal constraint to the constraints vector.
    factory->constraints.push_back( goalset );
}

void MotionOptimizer::finishGoalSet(){
    
    use_goalset = false;
    
    trajectory.endGoalSet();

    //remove the goal constraint, so that it is not deleted along
    //  with the other constraints.
    factory->constraints.pop_back();

}


void MotionOptimizer::setLowerBounds( const MatX & lower )
{
    assert( lower.size() == trajectory.M() );
    lower_bounds = lower;
}
void MotionOptimizer::setLowerBounds( const std::vector<double> & lower)
{
    assert( lower.size() == size_t(trajectory.M())  );
    lower_bounds = ConstMatMap(lower.data(), trajectory.M() , 1 );
}
void MotionOptimizer::setLowerBounds( const double * lower)
{
    lower_bounds = ConstMatMap(lower, trajectory.M() , 1 );
}


void MotionOptimizer::setUpperBounds(const MatX & upper )
{
    assert( upper.size() == trajectory.M()  );
    upper_bounds = upper;
}
void MotionOptimizer::setUpperBounds( const std::vector<double> & upper)
{
    assert( upper.size() == size_t(trajectory.M() ) );
    upper_bounds = ConstMatMap(upper.data(), trajectory.M(), 1 );
}
void MotionOptimizer::setUpperBounds( const double * upper)
{
    upper_bounds = ConstMatMap(upper, trajectory.M() , 1 );
}

void MotionOptimizer::setBounds( const MatX & lower, const MatX & upper )
{
    setLowerBounds( lower );
    setUpperBounds( upper );
}
void MotionOptimizer::setBounds( const std::vector<double> & lower, 
                                 const std::vector<double> & upper){
    setLowerBounds( lower );
    setUpperBounds( upper );
}
void MotionOptimizer::setBounds( const double* lower, 
                                    const double* upper){
    setLowerBounds( lower );
    setUpperBounds( upper );
}






}//namespace
