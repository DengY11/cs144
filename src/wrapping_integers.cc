#include "wrapping_integers.hh"
#include <vector>
#include <algorithm>
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
    constexpr uint64_t mod_value = static_cast<uint64_t>( 1 ) << 32;
    return Wrap32 { static_cast<uint32_t>( ( ( n + static_cast<uint64_t>( zero_point.raw_value_ ) ) % mod_value ) ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
    uint32_t offset = static_cast<uint32_t>( this->raw_value_ - zero_point.raw_value_ );

    constexpr uint64_t mod_value = static_cast<uint64_t>( 1 ) << 32;

    uint64_t base = static_cast<uint64_t>( checkpoint / mod_value ) * mod_value + offset;

    uint64_t candidate_1 = base;
    uint64_t candidate_2 = base + mod_value;
    uint64_t candidate_3 = base - mod_value;

    auto get_absolute_difference = [&checkpoint]( uint64_t value ) {
        if ( value >= checkpoint ) {
            return static_cast<uint64_t>( value - checkpoint );
        } else {
            return static_cast<uint64_t>( checkpoint - value );
        }
    };

    uint64_t absolute_difference_1 = get_absolute_difference( candidate_1 );
    uint64_t absolute_difference_2 = get_absolute_difference( candidate_2 );
    uint64_t absolute_difference_3 = get_absolute_difference( candidate_3 );

    uint64_t best_distance = absolute_difference_1;
    uint64_t best_candidate = candidate_1;

    if( absolute_difference_2 <= best_distance ) {
        best_distance = absolute_difference_2;
        best_candidate = candidate_2;
    }

    if( absolute_difference_3 <= best_distance ) {
        best_distance = absolute_difference_3;
        best_candidate = candidate_3;
    }

    return best_candidate;

}
