#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "evaluator.hh"
#include "network.cc"
#include "rat-templates.cc"

const unsigned int TICK_COUNT = 1000000;

Evaluator::Evaluator( const WhiskerTree & s_whiskers, const ConfigRange & range )
  : _prng( global_PRNG()() ), /* freeze the PRNG seed for the life of this Evaluator */
    _whiskers( s_whiskers ),
    _configs()
{
  /* sample 64 link speeds */

  const double steps = 64.0;

  const double link_speed_dynamic_range = range.link_packets_per_ms.second / range.link_packets_per_ms.first;

  const double multiplier = pow( link_speed_dynamic_range, 1.0 / steps );

  double link_speed = range.link_packets_per_ms.first;

  /* this approach only varies link speed, so make sure no
     uncertainty in rtt */
  assert( range.rtt_ms.first == range.rtt_ms.second );

  while ( link_speed <= (range.link_packets_per_ms.second * ( 1 + (multiplier-1) / 2 ) ) ) {
    _configs.push_back( NetConfig().set_link_ppt( link_speed ).set_delay( range.rtt_ms.first ).set_num_senders( range.max_senders ).set_on_duration( range.mean_on_duration ).set_off_duration( range.mean_off_duration ) );
    link_speed *= multiplier;
  }
}

Evaluator::Outcome Evaluator::score( const std::vector< Whisker > & replacements,
				     const bool trace, const unsigned int carefulness ) const
{
  PRNG run_prng( _prng );

  WhiskerTree run_whiskers( _whiskers );
  for ( const auto &x : replacements ) {
    assert( run_whiskers.replace( x ) );
  }

  run_whiskers.reset_counts();

  /* run tests */
  Outcome the_outcome;
  for ( auto &x : _configs ) {
    /* run once */
    Network<Rat> network1( Rat( run_whiskers, trace ), run_prng, x );
    network1.run_simulation( TICK_COUNT * carefulness );

    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }

  the_outcome.used_whiskers = run_whiskers;

  return the_outcome;
}
