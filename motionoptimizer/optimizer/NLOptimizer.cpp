
#include "NLOptimizer.h"
#include "../containers/ChompGradient.h"
#include "../constraint/ConstraintFactory.h"


namespace chomp {


const std::string getNLoptReturnString( nlopt::result & result ){
    if ( result == nlopt::SUCCESS ){ return "SUCCESS"; }
    if ( result == nlopt::STOPVAL_REACHED ){ return "STOPVAL_REACHED"; }
    if ( result == nlopt:: FTOL_REACHED ){ return "FTOL_REACHED"; }
    if ( result == nlopt::XTOL_REACHED  ){ return "XTOL_REACHED "; }
    if ( result == nlopt::MAXEVAL_REACHED ){ return "MAXEVAL_REACHED"; }
    if ( result == nlopt::MAXTIME_REACHED ){ return "MAXTIME_REACHED"; }
    return "UNKNOWN_RESULT";
}


NLOptimizer::NLOptimizer( Trajectory & traj,
                          ConstraintFactory * factory,
                          ChompGradient * gradient,
                          ChompObserver * observer,
                          double obstol,
                          double timeout_seconds,
                          size_t max_iter,
                          const MatX & lower_bounds,
                          const MatX & upper_bounds) : 

    OptimizerBase( traj, factory, gradient, observer,
                   obstol, timeout_seconds, max_iter,
                   lower_bounds, upper_bounds ),
    algorithm( nlopt::LD_MMA )
{
}


NLOptimizer::~NLOptimizer(){}

void NLOptimizer::solve()
{
    
    notify(CHOMP_INIT, 0, current_objective, -1, 0);
    
    //create the optimizer
    nlopt::opt optimizer( algorithm, trajectory.size() );

    //set termination conditions.
    if ( timeout_seconds <= 0){optimizer.set_maxtime(timeout_seconds);}
    if ( obstol <= 0 ){   optimizer.set_ftol_rel( obstol ); }
    if ( max_iter <= 0 ){ optimizer.set_maxeval( max_iter ); }

    //prepare the gradient for the run.
    gradient->prepareRun( trajectory );

    //prepare the constraints for optimization.
    //  This MUST be called before set_min_objective and giveBoundsToNLopt,
    //  because prepareNLoptConstraints can change the optimization
    //  routine.
    if ( factory ){  prepareNLoptConstraints( optimizer ); }
    giveBoundsToNLopt( optimizer );

    //set the objective function and the termination conditions.
    optimizer.set_min_objective( ChompGradient::NLoptFunction, gradient );
    
    //call the optimization routine, get the result and the value
    //  of the objective function.
    std::vector<double> optimization_vector;
    trajectory.copyDataTo( optimization_vector );
    
    //many of the optimizer algorithms fail, but still return very good
    //  trajectories, so catch the failures and return the trajectory,
    //  even if failure occurs.
    try{
        result = optimizer.optimize(optimization_vector, current_objective);
    }catch( std::exception & e ){
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
    

    trajectory.copyToData( optimization_vector );

    //notify the observer of the happenings.
    notify( CHOMP_FINISH, 0, current_objective, 
            last_objective, 0);
    std::cout << "Finished with exit code: "
              << getNLoptReturnString(result) << "\n";

}


void NLOptimizer::prepareNLoptConstraints( nlopt::opt & optimizer )
{

    //if there is no factory, do nothing
    if ( !factory ){ return; }

    //Get the dimensionality of the constraint, and fill the
    //  constraint_tolerances vector with the appropriate number of
    //  elements.
    int constraint_dims = factory->numOutput();

    if ( constraint_dims != 0 ){ 
        
        //the algorithm must be one that can handle equality constraints,
        //  if it is not one that can handle equality constraints,
        //  use the AUGLAG algorithm, with the specified local optimizer.
        if ( algorithm != nlopt::LD_SLSQP ){

            //create the new optimizer, and set the local optimizer.
            nlopt::opt new_optimizer( nlopt::AUGLAG, trajectory.size() );
            new_optimizer.set_local_optimizer( optimizer );

            optimizer = new_optimizer;
            if ( obstol != 0 ){ optimizer.set_ftol_rel( obstol ); }
            if ( max_iter != 0 ){ optimizer.set_maxeval( max_iter ); }

        }
        
        std::vector<double> constraint_tolerances;
        constraint_tolerances.resize( constraint_dims, 1e-5 );
        
        //pass the constraint function into nlopt.
        optimizer.add_equality_mconstraint(
                                ConstraintFactory::NLoptConstraint,
                                factory, constraint_tolerances);
    }

}


void NLOptimizer::copyNRows( const MatX & original_bounds, 
                             std::vector<double> & result)
{
    result.reserve( trajectory.size() );
    
    //eigen::matrices are stored in column major format
    for ( int i = 0; i < trajectory.M(); i ++ ){
        result.resize( result.size()+trajectory.N(),
                       original_bounds(i) );
    }
}

void NLOptimizer::giveBoundsToNLopt( nlopt::opt & optimizer )
{
    
    //set the lower bounds if the lower vector is 
    //  of the correct size.
    if ( lower_bounds.size() == trajectory.M() ){
        std::vector<double> nlopt_lower_bounds;
        copyNRows( lower_bounds, nlopt_lower_bounds );
        
        optimizer.set_lower_bounds( nlopt_lower_bounds );
    }

    //set the upper bounds if the upper matrix is of the 
    //  correct size.
    if ( upper_bounds.size() == trajectory.M() ){

        std::vector<double> nlopt_upper_bounds;
        copyNRows( upper_bounds , nlopt_upper_bounds );
        
        optimizer.set_upper_bounds( nlopt_upper_bounds );
    }
}


}//namespace


