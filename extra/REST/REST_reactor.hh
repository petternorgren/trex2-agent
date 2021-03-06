/*********************************************************************
 * Software License Agreement (BSD License)
 * 
 *  Copyright (c) 2011, MBARI.
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the TREX Project nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef H_trex_REST_reactor
# define H_trex_REST_reactor

# include <trex/transaction/TeleoReactor.hh>
# include <trex/Wt/server.hh>

# include <boost/signals2/signal.hpp>

namespace TREX {
  namespace REST {
    
    class service_tree;
    class TimelineHistory;
    class tick_manager;
    
    class REST_reactor :public transaction::TeleoReactor {
    public:
      REST_reactor(TREX::transaction::TeleoReactor::xml_arg_type arg);
      ~REST_reactor();
      
    private:
      //reactor handlers
      void handleInit();
      void handleTickStart();
      void notify(TREX::transaction::Observation const &obs);
      bool synchronize();
      void newPlanToken(transaction::goal_id const &t);
      void cancelledPlanToken(transaction::goal_id const &t);
      
      TREX::utils::SingletonUse<TREX::wt::server> m_server;
      
      // UNIQ_PTR<Wt::WServer>     m_server;
      
      SHARED_PTR<TimelineHistory> m_timelines;
      SHARED_PTR<tick_manager>    m_tick;
      UNIQ_PTR<service_tree>             m_services;
      
      friend class TimelineHistory;
    };
    
  }
}

#endif // H_trex_REST_reactor
