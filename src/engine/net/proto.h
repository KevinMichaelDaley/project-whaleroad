#include "common/constants.h"
#include "extern/Float16Compressor/Float16Compressor.h"
#include <cstdint>

struct packet_block_update{
    uint8_t tick_index;
    uint16_t block_update_index;//if this increments by more than one, we know we lost something and need to re-request it.
    //our block update index is always client_id (mod MAX_CLIENT_ID).
    int32_t x,y;
    uint16_t z;
    block_t block;
};


struct packet_world_checksum{
    uint8_t tick_index;
    int32_t x0,y0;
    uint16_t z0;
    uint32_t blk_checksum;
    uint32_t lux_checksum;
};

struct packet_column_rle{
    uint8_t tick_index;
    int32_t x0,y0;
}; //N runs will be read sequentially from the next N*4 bytes using the following struct:

struct run_of_blocks{
    uint16_t length;
    int16_t block_value;
};

struct packet_entity_state_update{
    uint8_t tick_index;//resets every 64*constants::TICK_RATE ms.
    uint16_t time; //resets every couple seconds to avoid precision loss.
    uint16_t entity_id_lo;//lowest two bits reserved for sign of orientation w-component and look vector forward (y) component in local space
    uint8_t entity_id_hi;
    uint8_t anim_state;//for animation, etc.
    int x,y; 
    uint16_t z;//block coordinates must be absolute in every update; if send deltas and we lose a packet, everything gets messed up.
    uint8_t xsub,ysub,zsub;//0-1 normalized 8-bit value for offset from lower left bottom corner of block.
    uint16_t orientation[3];//orientation as binary16
    uint8_t look_x,look_z;//view direction in model space as normalized float
};

struct packet_act{
    uint8_t input_action;
    uint16_t tgtx[3];
};

enum class message_server: char{
    MSG_BLK_UPD,
    MSG_RLE,
    MSG_REQ,
    MSG_ENT_UPD
};
enum class message_client: char{
    MSG_BLK_UPD,    
    MSG_CHKSUM,
    MSG_REQ,
    MSG_ACT
};

class udp_transmitter{
    udp_connection* conn;
    int num_bytes_filled;
    int current_tick;
    double time_last_flush;
    int max_datagram_size;
    udp_transmitter(udp_connection* conn_, int max_dgram_size): conn(conn_), max_datagram_size(max_dgram_size), 
                                                                num_bytes_filled(0), current_tick(0), time_last_flush(0){
        current_tick=std::floor(timer::now()/constants::NET_TICK_RATE_HZ);
        time_last_flush=timer::now();
    }
    void append(char* upd, size_t sz){
        if(num_bytes_filled+sz>=max_datagram_size){
            flush();
        }
        std::memcpy((char*) &(send_buf[num_bytes_filled]), upd, sz);
        num_bytes_filled+=sz;
    }
    void flush(){
        conn->send_udp(buf, num_bytes_filled);
        time_last_flush=timer::now();
    }
    virtual void unpack_update(char* dgram, int& offset)=0;
    void recv_loop(){
        char dgram[max_datagram_size];
        size_t sz=srv->recv_udp(max_datagram_size);
        int offset=0;
        while(offset<sz){
            unpack_update(dgram, offset);
        }
    }
};
    
class client: public udp_transmitter{
    
    packet_block_update client_upds[64];
    client(udp_connection* srv_, int udp_mtu): udp_transmitter(srv_, udp_mtu-constants::UDP_HEADER_SIZE-constants::IP_HEADER_SIZE){
    }
    void unpack_update(char* dgram, int& offset){
        if(dgram[0]&3==message_server::MSG_BLK_UPD){
            packet_block_update x=*(packet_block_update*) &(dgram[offset]);
            handle_block_update(x);
            offset+=sizeof(x);
        }
        else if(dgram[0]&3==MSG::ENT_UPD){
            packet_physics_state_update x=*(packet_physics_state_update*) &(dgram[offset]);
            handle_state_update(x);
            offset+=sizeof(x);
        }
        else if(dgram[0]&3==MSG_RLE){
            packet_column_rle x=*(packet_column_rle*) &dgram[offset];
            handle_column_update(x);
            offset+=sizeof(x);
        }
        else if(dgram[0]&3==MSG_REQ){
            resend_block_update(client_upds[dgram[0]%64]);
        }
    }
    
   
};
class server{
//     
    packet_block_update server_upds[16];
    static packet_entity_state_update pack_entity_state(const character* ent){
        Vector3 pos=ent->get_position();
        int id=ent->get_id();
        float rx,ry,rz;
        ent->dir(rx,ry,rz);
        Quaternion q=ent->get_orientation();
        id+=(q.scalar()>0)<<1;
        Vector3 look=q.invertedNormalized().transformVectorNormalized(Vector3{rx,ry,rz});
        float look_x=(look.x()/look.length());
        float look_z=(look.z()/look.length());
        id+=(look.y()>0);
        Vector3 qv=q.vector();
        int bx=
            std::floor(pos.x()),
            by=
            std::floor(pos.y()),
            bz=
            std::floor(pos.z());
        packet_physics_state_update upd{
            uint8_t(current_tick%64u)<<uint8_t(2u)+uint8_t(3u),
            Float16Compressor::compress(std::fmod(timer::now(),constants::NETWORK_TIMER_RESET_INTERVAL)/constants::NETWORK_TIMER_RESET_INTERVAL),
            id&65535,
            id>>16,
            ent->get_anim_state(),
            bx,by,uint8_t(bz),
            uint8_t((pos.x()-bx)*255.0),
            uint8_t((pos.y()-by)*255.0),
            uint8_t((pos.z()-bz)*255.0),
            {
            Float16Compressor::compress(qv.x()),
            Float16Compressor::compress(qv.y()),
            Float16Compressor::compress(qv.z())},
            look_x*255.0,
            look_z*255.0
        };
        return upd;
    }
    void add_state_update(const character* ent){
        packet_entity_state_update upd=pack_entity_state(ent);
        append(&upd, sizeof(pack_entity_state));
    }        
    
    virtual void unpack_update(char* dgram, int& offset){
        if(dgram[0]&3==message_server::MSG_BLK_UPD){
            packet_block_update x=*(packet_block_update*) &(dgram[offset]);
            handle_block_update(x);
            offset+=sizeof(x);
        }
        else if(dgram[0]&3==message_server::MSG_CHKSUM){
            packet_world_checksum x=*(packet_world_checksum*) &(dgram[offset]);
            handle_checksum(x);
            offset+=sizeof(x);
        }
        else if(dgram[0]&3==message_server::MSG_REQ){
            uint16_t x=*(uint16_t*) &(dgram[offset]);
            handle_block_update_request(x);
            offset+=sizeof(x);
        }
        else if(dgram[0]&3==message_server::MSG_ACT){
            packet_player_input_event x=*(packet_player_input_event*) &(dgram[offset]);
            handle_player_input_event(x);
            offset+=sizeof(x);
        }
    }
};
    
