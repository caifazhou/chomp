/*
* Copyright (c) 2008-2014, Matt Zucker and Temple Price
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

#ifndef _CONSTRAINT_FACTORY_H_
#define _CONSTRAINT_FACTORY_H_

#include "Constraint.h"
#include "Trajectory.h"

namespace mopt {

class ConstraintFactory {


private:
    class ConstraintInterval {
        
      public:
            
        double start, stop;
        Constraint * constraint;

        //this is a function for comparing two intervals in
        //  sorting.
        inline static bool compare( ConstraintInterval first, 
                                    ConstraintInterval second )
        {
            return first.start < second.start;
        }

    };

private:

    std::vector<ConstraintInterval> constraint_intervals;
    bool interval_is_sorted;

    std::vector<Constraint*> constraints ;

    int constraint_dims;

    static const char* TAG;

public:

    ConstraintFactory(); 
    ~ConstraintFactory(); 

    void addConstraint( Constraint * constraint, 
                        double start,
                        double stop );

    void addGoalset( Constraint * goalset );
    void removeGoalset();
    bool empty() const;
    
    Constraint* createConstraint(size_t t, size_t total);

    Constraint* getConstraint( size_t t ) const;    
    const std::vector<Constraint*> & getConstraints() const;
    
    //TODO consider renaming this to prepareRun, for
    //  consistency with ConstraintFactory
    void getAll(size_t total);

    int numOutput();    
    
    template <class Derived1, class Derived2>
    double evaluate( const Trajectory & trajectory,
                     const Eigen::MatrixBase<Derived1> & h_tot,
                     const Eigen::MatrixBase<Derived2> & H_tot);

    
    template <class Derived>
    double evaluate(const Trajectory & trajectory,
                    const Eigen::MatrixBase<Derived> & h_tot);


private:
    void sortIntervals();
    
    //calculates the dimensionality of the whole constraint
    //  vector.
    void calculateConstraintDimension();

};

//include the template functions implementation
#include "ConstraintFactory-inl.h"


}//namespace
#endif
