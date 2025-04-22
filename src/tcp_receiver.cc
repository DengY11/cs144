#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{

    if ( message.RST ) {
        this->reassembler_.reader().set_error();
    }

    std::string&& payload = std::move( message.payload );

    bool SYN = message.SYN;

    if ( SYN && !this->syn_received_ ) {
        this->zero_point_ = message.seqno;
        syn_received_ = true;
    }

    if ( !syn_received_ ) {
        return;
    }

    uint64_t check_point = this->reassembler_.writer().bytes_pushed();

    uint64_t abs_seqno = message.seqno.unwrap( this->zero_point_, check_point );

    size_t index = SYN ? 0 : ( abs_seqno - 1 );

    this->reassembler_.insert( index, std::move( payload ), message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
    TCPReceiverMessage message;
    if ( !this->syn_received_ ) {
        message.ackno = std::nullopt;
    } else {
        uint64_t next_abs = reassembler_.writer().bytes_pushed() + 1;
        if ( reassembler_.writer().is_closed() ) {
            next_abs += 1;
        }
        message.ackno = Wrap32::wrap( next_abs, zero_point_ );
    }

    std::size_t capacity = this->reassembler_.writer().available_capacity();
    message.window_size = static_cast<uint16_t>( std::min( capacity, static_cast<size_t>( UINT16_MAX ) ) );

    message.RST = reassembler_.writer().has_error();
    return message;
}
