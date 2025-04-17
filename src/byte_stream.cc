#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
    uint64_t data_size = data.size();
    uint64_t len_to_push = std::min( data_size, available_capacity() );

    if ( is_closed() || has_error() || len_to_push == 0 ) {
        return;
    }

    data_.insert( data_.end(), data.begin(), data.begin() + len_to_push );
    bytes_pushed_cnt_ += len_to_push;
}

void Writer::close()
{
    is_closed_ = true;
}

bool Writer::is_closed() const
{
    return this->is_closed_;
}

uint64_t Writer::available_capacity() const
{
    return capacity_ - data_.size();
}

uint64_t Writer::bytes_pushed() const
{
    return bytes_pushed_cnt_;
}

string_view Reader::peek() const
{
    return std::string_view { &data_.front(), 1 };
}

void Reader::pop( uint64_t len )
{
    if ( len > bytes_buffered() ) {
        set_error();
        return;
    }

    for ( uint64_t i = 0; i < len; i++ ) {
        data_.pop_front();
    }

    bytes_popped_cnt_ += len;
}

bool Reader::is_finished() const
{
    return is_closed_ and data_.empty();
}

uint64_t Reader::bytes_buffered() const
{
    return data_.size();
}

uint64_t Reader::bytes_popped() const
{
    return bytes_popped_cnt_;
}
