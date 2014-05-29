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
#include "reactor_graph.hh"
#include "private/graph_impl.hh"
#include "reactor.hh"

#include <boost/date_time/posix_time/posix_time_io.hpp>

using namespace TREX::transaction;
namespace utils=TREX::utils;

namespace asio=boost::asio;


namespace {



  class DateHandler :public abstract_domain::factory::factory_type::producer {
    typedef abstract_domain::factory::factory_type::producer base_class;

  public:
    typedef graph::date_type date_type;

    ~DateHandler() {} 

    DateHandler(utils::symbol const &tag, graph const &owner)
    :base_class(tag), m_owner(owner) {
      base_class::notify();
    }

  private:  
    base_class::result_type produce(base_class::argument_type arg) const {
      boost::optional<date_type> 
      min = utils::parse_attr< boost::optional<date_type> >(arg.second, "min"),
      max = utils::parse_attr< boost::optional<date_type> >(arg.second, "max");
      int_domain::bound lo(int_domain::minus_inf),
          hi(int_domain::plus_inf);
      boost::posix_time::ptime date;
      if( min )
        lo = m_owner.time_to_tick(*min);
      if( max )
        hi = m_owner.time_to_tick(*max);
      return result_type(new int_domain(lo, hi));
    }

    graph const &m_owner;

    friend class TREX::transaction::graph;
  }; // DateHandler

  class DurationHandler :public abstract_domain::factory::factory_type::producer {
    typedef abstract_domain::factory::factory_type::producer base_class;

  public:
    ~DurationHandler() {} 

    DurationHandler(utils::symbol const &tag, graph const &owner)
    :base_class(tag), m_owner(owner) {
      base_class::notify();
    }

  private:
    base_class::result_type produce(base_class::argument_type arg) const {
      boost::optional<utils::rt_duration>
      min = utils::parse_attr< boost::optional<utils::rt_duration> >(arg.second, "min"),
      max = utils::parse_attr< boost::optional<utils::rt_duration> >(arg.second, "max");
      int_domain::bound lo(int_domain::minus_inf),
          hi(int_domain::plus_inf);
      CHRONO::duration<double> ratio = m_owner.tick_duration();
      
      if( min ) {
        CHRONO::duration<double> min_s = min->to_chrono< CHRONO::duration<double> >();
        lo = static_cast<long long>(std::floor(min_s.count()/ratio.count()));
      }
      if( max ) {
        CHRONO::duration<double> max_s = max->to_chrono< CHRONO::duration<double> >();
        lo = static_cast<long long>(std::ceil(max_s.count()/ratio.count()));
        
      }
      return result_type(new int_domain(lo, hi));
    }

    graph const &m_owner;

    friend class TREX::transaction::graph;
  }; // DurationHandler

}

/*
 * class TREX::transaction::GraphException
 */
GraphException::GraphException(graph const &g, std::string const &msg) throw()
      :TREX::utils::Exception(g.name().str()+": "+msg) {}

GraphException::GraphException(graph const &g, std::string const &who,
    std::string const &msg) throw()
      :TREX::utils::Exception(g.name().str()+"."+who+": "+msg) {}


/*
 * class TREX::transaction::MultipleReactors
 */
MultipleReactors::MultipleReactors(graph const &g, reactor const &r) throw()
      :GraphException(g, "Multiple reactors with the same name \""+
          r.name().str()+"\"") {}

/*
 * class TREX::transaction::graph
 */

// structors :

graph::graph()
:m_impl(MAKE_SHARED<details::graph_impl>())
{}

graph::graph(utils::symbol const &name, TICK init, bool verbose)
:m_impl(MAKE_SHARED<details::graph_impl>(name))
, m_currentTick(init)
, m_verbose(verbose) {}

graph::graph(utils::symbol const &name, boost::property_tree::ptree &conf,
    TICK init, bool verbose)
:m_impl(MAKE_SHARED<details::graph_impl>(name))
, m_currentTick(init), m_verbose(verbose) {
  size_t number = add_reactors(conf);
  syslog(info)<<"Created "<<number<<" reactors.";
}

graph::~graph() {
  clear();
  manager().flush();
}

TREX::utils::log::stream graph::syslog(utils::symbol const &context,
    utils::symbol const &kind) const {
  return m_impl->syslog(context, kind);
}

bool graph::is_isolated(graph::reactor_id r) const {
  return m_quarantined.find(r->name())!=m_quarantined.end();
}

std::string graph::date_str(TICK cur) const {
  std::ostringstream oss;
  oss<<cur;
  return oss.str();
}

std::string graph::duration_str(TICK dur) const {
  return date_str(dur);
}

TREX::utils::symbol const &graph::name() const {
  return m_impl->name();
}

void graph::set_name(TREX::utils::symbol const &name) {
  m_impl->set_name(name);
}

bool graph::has_tick() const {
  return m_impl->date();
}

void graph::update_tick(TICK value, bool started) {
  if( started ) {
    m_tick_updated = true;
    m_impl->set_date(value);
  } else
    m_currentTick = value;
}

TICK graph::current_tick() const {
  if( tick_updated() ) {
    boost::optional<details::date_type> cur = m_impl->date();
    m_tick_updated = false;
    if( cur )
      m_currentTick = *cur;
  }
  return m_currentTick;
}

boost::asio::strand &graph::strand() {
  return m_impl->strand();
}

TREX::utils::log_manager &graph::manager() const {
  return m_impl->manager();
}


void graph::clear() {
  while( !m_reactors.empty() ) {
    syslog(info)<<"Disconnecting \""<<m_reactors.front()->name()
		    <<"\" from the graph.";
    m_reactors.front()->isolate(false);
    m_reactors.pop_front();
  } 
  m_quarantined.clear();
}

long graph::index(graph::reactor_id id) const {
  size_t pos = 0;
  // not efficient at all !!!
  details::reactor_set::const_iterator i;
  for(i=m_reactors.begin();
      m_reactors.end()!=i && i->get()!=id; ++i, ++pos);
  return pos;
}

graph::reactor_id graph::add_reactor(boost::property_tree::ptree::value_type &description) {
  graph *me = this;
  reactor::xml_arg_type
  arg = factory::arg_traits::build(description, me);
  SHARED_PTR<reactor> tmp(m_factory->produce(arg));
  std::pair<details::reactor_set::iterator, bool> ret = m_reactors.insert(tmp);

  if( ret.second ) 
    syslog(info)<<"Reactor \""<<tmp->name()<<"\" created.";
  else
    throw MultipleReactors(*this, **(ret.first));			   
  return ret.first->get();
}

graph::reactor_id graph::add_reactor(graph::reactor_id r) {
  SHARED_PTR<reactor> tmp(r);
  std::pair<details::reactor_set::iterator, bool> ret = m_reactors.insert(tmp);
  // As it is an internal call make is silent for now ...
  return ret.first->get();
}

bool graph::is_member(graph::reactor_id r) const {
  for(details::reactor_set::const_iterator i=m_reactors.begin();
      m_reactors.end()!=i; ++i)
    if( i->get() == r )
      return true;
  return false;
}

bool graph::kill_reactor(graph::reactor_id r) {
  if( null_reactor()!=r ) {
    details::reactor_set::iterator pos = m_reactors.find(r->name()),
        pos_q = m_quarantined.find(r->name());

    if( pos->get()==r ) {
      // clean up relations
      syslog(warn)<<"Destroying reactor \""<<r->name()<<"\".";
      if( pos_q->get()==r )
        m_quarantined.erase(pos_q);
      else 
        r->isolate();
      // std::cerr<<"Erase the reactor"<<std::endl;
      m_reactors.erase(pos);
      /// std::cerr<<"Done."<<std::endl;
      return true;
    }
  }
  return false;
}

graph::reactor_id graph::isolate(graph::reactor_id r) const {
  if( null_reactor()!=r ) {
    details::reactor_set::const_iterator pos = m_reactors.find(r->name());
    if( pos->get()==r ) {
      syslog(info)<<"Putting reactor\""<<r->name()<<"\" in quarantine.";
      r->isolate();
      m_quarantined.insert(*pos);
      return r;
    }
  }
  return null_reactor();
}

size_t graph::cleanup() {
  size_t ret =0;
  for(; !m_quarantined.empty(); ++ret)
    kill_reactor(m_quarantined.front().get());
  return ret;
}  

details::timeline_set::iterator graph::get_timeline(utils::symbol const &tl) {
  details::timeline *cand = new details::timeline(m_currentTick, tl);
  std::pair<details::timeline_set::iterator, bool>
  ret = m_timelines.insert(cand);
  if( ret.second ) 
    syslog(info)<<"Timeline \""<<tl.str()<<"\" created.";
  else
    delete cand;
  return ret.first;
}

graph::size_type graph::count_relations() const {
  size_t res = 0;
  for(details::timeline_set::const_iterator i=m_timelines.begin();
      m_timelines.end()!=i; ++i )
    res += (*i)->size();
  return res;
}


bool graph::assign(graph::reactor_id r, utils::symbol const &timeline, details::transaction_flags const &flags) {
  details::timeline_set::iterator tl = get_timeline(timeline);
  try {
    internal_check(r, **tl);
    return (*tl)->assign(*r, flags);
  } catch(timeline_failure const &err) {
    return r->failed_internal(timeline, err);
  }
}

bool graph::subscribe(reactor_id r, utils::symbol const &timeline, details::transaction_flags const &flags) {
  details::timeline_set::iterator tl = get_timeline(timeline);
  try {
    external_check(r, **tl);
    return (*tl)->subscribe(*r, flags);
  } catch(timeline_failure const &err) {
    return r->failed_external(timeline, err);
  }
}

goal_id graph::parse_goal(boost::property_tree::ptree::value_type goal) const {
  DateHandler date_parser("date", *this);
  DurationHandler duration_parser("duration", *this);

  return goal_id(new Goal(goal));
}

namespace bp=boost::property_tree;

namespace TREX {
  namespace transaction {

    std::string date_export(graph const &g, int_domain::bound const &val) {
      return g.date_str(val.value());
    }

    bp::ptree date_export(graph const &g, int_domain const &dom) {
      bp::ptree ret;
      bp::ptree &tmp = ret.add_child("date", bp::ptree());

      if( dom.is_singleton() )
        TREX::utils::set_attr(tmp, "value", date_export(g,dom.lower_bound()));
      else {
        if( dom.has_lower() )
          TREX::utils::set_attr(tmp, "min", date_export(g,dom.lower_bound()));
        if( dom.has_upper() )
          TREX::utils::set_attr(tmp, "max", date_export(g,dom.upper_bound()));
      }
      TREX::utils::set_attr(ret, "type", "date");
      return ret;
    }

    std::string duration_export(graph const &g,
        int_domain::bound const &val) {
      return g.duration_str(val.value());
    }

    bp::ptree duration_export(graph const &g, int_domain const &dom) {
      bp::ptree ret;
      bp::ptree &tmp = ret.add_child("duration", bp::ptree());

      if( dom.is_singleton() )
        TREX::utils::set_attr(tmp, "value",
            duration_export(g,dom.lower_bound()));
      else {
        if( dom.has_lower() )
          TREX::utils::set_attr(tmp, "min",
              duration_export(g,dom.lower_bound()));
        if( dom.has_upper() )
          TREX::utils::set_attr(tmp, "max",
              duration_export(g,dom.upper_bound()));
      }
      TREX::utils::set_attr(ret, "type", "duration");
      return ret;
    }
  }
}

TICK graph::as_date(std::string const &str) const {
  return time_to_tick(boost::lexical_cast<date_type>(str));
}

TICK graph::as_duration(std::string const &str, bool up) const {
  utils::rt_duration tmp = boost::lexical_cast<utils::rt_duration>(str);
  
  CHRONO::duration<double> ratio = tick_duration(),
  val = tmp.to_chrono< CHRONO::duration<double> >();
  
  double value = val.count()/ratio.count();
    
  if( up )
    return static_cast<TICK>(std::ceil(value));
  else
    return static_cast<TICK>(std::floor(value));
}


boost::property_tree::ptree graph::export_goal(goal_id const &g) const {
  bp::ptree ret = g->as_tree(false);
  bp::ptree &attr = ret.front().second;
  boost::optional<bp::ptree &> vars = attr.get_child_optional("Variable");

  if( !vars )
    vars = attr.add_child("Variable", bp::ptree());

  if( !g->getStart().is_full() ) {
    bp::ptree domain = date_export(*this, g->getStart());
    TREX::utils::set_attr(domain, "name", "start");
    vars->push_back(bp::ptree::value_type("", domain));
  }
  if( !g->getEnd().is_full() ) {
    bp::ptree domain = date_export(*this, g->getEnd());
    TREX::utils::set_attr(domain, "name", "end");
    vars->push_back(bp::ptree::value_type("", domain));
  }
  if( g->getDuration().has_upper() ||
      g->getDuration().lower_bound()!=Goal::s_durationDomain.lower_bound() ) {
    bp::ptree domain = duration_export(*this, g->getDuration());
    TREX::utils::set_attr(domain, "name", "duration");
    vars->push_back(bp::ptree::value_type("", domain));    
  }

  return ret;
}



/*
 * class TREX::transaction::graph::timelines_listener
 */

// structors 

graph::timelines_listener::timelines_listener(graph &g)
:m_graph(g) {
  m_graph.m_listeners.insert(this);
}

graph::timelines_listener::timelines_listener(graph::factory::argument_type const &arg)
:m_graph(*(arg.second)){
  m_graph.m_listeners.insert(this);  
}

graph::timelines_listener::~timelines_listener() {
  m_graph.m_listeners.erase(this);
}

// manpipulators

void graph::timelines_listener::initialize() {
  for(details::timeline_set::const_iterator tl=m_graph.m_timelines.begin();
      m_graph.m_timelines.end()!=tl; ++tl) {
    // declare the timeline 
    declared(**tl);
    // declare its connections
    for(Relation r=(*tl)->begin(); (*tl)->end()!=r; ++r) 
      connected(r);
  }
}

