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

        // å¦‚æž
        if ( next_index_ == eof_index_ ) {
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
    strPtr segment_ptr = std::make_shared<std::string>( std::move( segment ) );

    uint64_t new_l = seg_start, new_r = seg_end;

    auto it = stream_buf_.lower_bound( new_l );

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

    if ( it != stream_buf_.begin() ) {
        auto prev = std::prev( it );
        uint64_t prev_l = prev->first;
        uint64_t prev_r = prev_l + prev->second->size();

        if ( prev_r >= new_l ) {
            new_l = std::min( new_l, prev_l );
            new_r = std::max( new_r, prev_r );
            std::string merged( new_r - new_l, '\0' );
            strPtr merged_ptr = std::make_shared<std::string>( std::move( merged ) );

            memcpy( &( merged_ptr.get()->data()[0] ), prev->second->data(), prev->second->size() );
            memcpy( &( merged_ptr.get()->data()[seg_start - new_l] ), segment_ptr->data(), segment_ptr->size() );

            segment_ptr.swap( merged_ptr );
            seg_start = new_l;
            it = stream_buf_.erase( prev );
        }
    }

    while ( it != stream_buf_.end() and it->first <= new_r ) {
        uint64_t cur_l = it->first;
        uint64_t cur_r = cur_l + it->second->size();

        new_r = std::max( new_r, cur_r );
        std::string merged( new_r - new_l, '\0' );
        strPtr merged_ptr = std::make_shared<std::string>( std::move( merged ) );

        memcpy( &( merged_ptr.get()->data()[0] ), segment_ptr.get()->data(), segment_ptr.get()->size() );
        memcpy( &( merged_ptr.get()->data()[cur_l - new_l] ), it->second->data(), it->second->size() );

        segment_ptr.swap( merged_ptr );
        it = stream_buf_.erase( it );
    }

    stream_buf_[new_l] = std::move( segment_ptr );

    while ( !stream_buf_.empty() ) {
        auto iter = stream_buf_.begin();
        uint64_t l = iter->first;
        if ( l != next_index_ ) {
            break;
        }

        auto seg = std::move( iter->second );
        stream_buf_.erase( iter );

        std::size_t write_len = std::min( seg->size(), output_.writer().available_capacity() );
        next_index_ += write_len;

        if ( write_len == seg->size() ) {
            output_.writer().push( std::move( *seg ) );
        } else {
            std::string&& push_str = seg->substr( 0, write_len );
            output_.writer().push( std::move( push_str ) );
            strPtr remain_ptr = std::make_shared<std::string>( seg->substr( write_len ) );
            stream_buf_[next_index_] = std::move( remain_ptr );
        }
    }

    if ( eof_marked_ and next_index_ == eof_index_ ) {
        output_.writer().close();
    }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
    uint64_t count = 0;
    for ( const auto& it : stream_buf_ ) {
        count += it.second->size();
    }
    return count;
}
