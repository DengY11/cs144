#include "reassembler.hh"
#include "debug.hh"
#include <algorithm>
#include <cassert>
#include <string.h>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
    if ( is_last_substring ) {
        eof_marked_ = true;
        eof_index_ = first_index + data.size();

        // 如果最后插入一个空串，应该在这里关闭流
        if(next_index_ == eof_index_) {
            output_.writer().close();
            return;
        }
    }

    uint64_t stream_cap = output_.writer().available_capacity();
    uint64_t write_up_to = next_index_ + stream_cap;
    uint64_t seg_start = std::max( first_index, next_index_ );
    uint64_t seg_end = std::min( first_index + data.size(), write_up_to );

    if ( seg_end <= seg_start ) {
        return;
    }

    /*
     *
     *               ------------------------------    data
     *     ------------------------                    stream
     *     ^         ^             ^              ^
     *     |         \             |              |
     *     prev_l    \           next_index_      seg_end
     *               first_index   |              |
     *                            seg_start       |
     *                             |              |
     *                             -----------------    segment
     * */
    std::string segment = data.substr( seg_start - first_index, seg_end - seg_start );

    uint64_t new_l = seg_start, new_r = seg_end;

    auto it = buffer_.lower_bound( new_l );

    /*
     *
     *
     *
     *
     *
     *                   ------------------- segment
     *                   ^                 ^
     *                   |                 |
     *                   new_l             new_r 
     *
     * prev -----------------           -------------------  lower_bound
     *      ^               ^                            
     *      |               |                                            
     *      prev_l         prev_r
     *
     *      -------------------------------- merged
     *
     * */

    if ( it != buffer_.begin() ) {
        auto prev = std::prev( it );
        uint64_t prev_l = prev->first;
        uint64_t prev_r = prev_l + prev->second.size();

        if ( prev_r >= new_l ) {
            new_l = std::min( new_l, prev_l );
            new_r = std::max( new_r, prev_r );
            std::string merged( new_r - new_l, '\0' );

            memcpy( &merged[prev_l - new_l], prev->second.data(), prev->second.size() );
            memcpy( &merged[seg_start - new_l], segment.data(), segment.size() );

            segment.swap( merged );
            seg_start = new_l;
            it = buffer_.erase( prev );
        }
    }

    while ( it != buffer_.end() and it->first <= new_r ) {
        uint64_t cur_l = it->first;
        uint64_t cur_r = cur_l + it->second.size();

        new_r = std::max( new_r, cur_r );
        std::string merged( new_r - new_l, '\0' );

        memcpy( &merged[seg_start - new_l], segment.data(), segment.size() );
        memcpy( &merged[cur_l - new_l], it->second.data(), it->second.size() );

        segment.swap( merged );
        it = buffer_.erase( it );
    }

    buffer_[new_l] = std::move( segment );

    while ( !buffer_.empty() ) {
        auto iter = buffer_.begin();
        uint64_t l = iter->first;
        auto& seg = iter->second;
        if ( l != next_index_ ) {
            break;
        }

        std::size_t write_len = std::min( seg.size(), output_.writer().available_capacity() );
        output_.writer().push( seg.substr( 0, write_len ) );
        next_index_ += write_len;
        if ( write_len == seg.size() ) {
            buffer_.erase( iter );
        } else {
            std::string remain( seg.substr( write_len ) );
            buffer_.erase( iter );
            buffer_[next_index_] = std::move( remain );
        }
    }

    if ( eof_marked_ and next_index_ == eof_index_ ) {
        output_.writer().close();
    }

    // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
    // 我他妈的好狠世界！！
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
    uint64_t count = 0;
    for ( const auto& it : buffer_ ) {
        count += it.second.size();
    }
    return count;
}
