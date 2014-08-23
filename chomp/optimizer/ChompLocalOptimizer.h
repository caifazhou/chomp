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

#ifndef _CHOMP_LOCAL_OPTIMIZER_H_
#define _CHOMP_LOCAL_OPTIMIZER_H_

#include <vector>
#include <pthread.h>

#include "chomputil.h"
#include "ChompGradient.h"
#include "ChompOptimizerBase.h"
#include "../mzcommon/TimeUtil.h"

namespace chomp {

class ChompLocalOptimizer : public ChompOptimizerBase {
  public:

    ChompLocalOptimizer( ConstraintFactory * factory,
                         ChompGradient * gradient,
                         ChompObserver * observer=NULL,
                         double obstol = 1e-8,
                         double timeout_seconds = 0,
                         size_t max_iter = size_t(-1),
                         const MatX & lower_bounds=MatX(0,0),
                         const MatX & upper_bounds=MatX(0,0)); 

  protected:
    virtual void optimize( Trajectory & xi );

};

}//Namespace


#endif
