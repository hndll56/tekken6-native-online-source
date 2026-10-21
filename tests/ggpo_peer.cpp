#ifdef _WIN32
#include <winsock2.h>
#endif
#include <ggponet.h>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
struct State { int frame; uint32_t checksum; };
static State state{};
static GGPOSession* session;
static bool running=false;
static int loads=0;
static bool begin(const char*) { return true; }
static bool save(unsigned char** b, int* len, int* sum, int) {
    *len=sizeof(state); *b=static_cast<unsigned char*>(malloc(*len)); memcpy(*b,&state,*len); *sum=int(state.checksum); return true;
}
static bool load(unsigned char* b, int) { memcpy(&state,b,sizeof(state)); ++loads; return true; }
static void freeBuffer(void* b) { free(b); }
static bool event(GGPOEvent* e) { if(e->code==GGPO_EVENTCODE_RUNNING) running=true; return true; }
static bool advance(int) {
    uint8_t input[24]{}; int disconnect=0;
    if (ggpo_synchronize_input(session,input,sizeof(input),&disconnect)!=GGPO_OK) return false;
    for (auto b:input) state.checksum=(state.checksum*16777619u)^b;
    ++state.frame; return ggpo_advance_frame(session)==GGPO_OK;
}
int main(int argc,char** argv) {
    if(argc!=4) return 2;
#ifdef _WIN32
    WSADATA winsock{}; if(WSAStartup(MAKEWORD(2,2),&winsock)) return 7;
#endif
    int player=atoi(argv[1]);
    GGPOSessionCallbacks cb{}; cb.begin_game=begin; cb.save_game_state=save; cb.load_game_state=load;
    cb.free_buffer=freeBuffer; cb.advance_frame=advance; cb.on_event=event;
    if(ggpo_start_session(&session,&cb,"linux-port-test",2,12,atoi(argv[2]))!=GGPO_OK) return 3;
    GGPOPlayerHandle local=-1;
    for(int p=1;p<=2;++p) {
        GGPOPlayer desc{}; desc.size=sizeof(desc); desc.player_num=p;
        desc.type=p==player?GGPO_PLAYERTYPE_LOCAL:GGPO_PLAYERTYPE_REMOTE;
        if(p!=player) { strcpy(desc.u.remote.ip_address,"127.0.0.1"); desc.u.remote.port=atoi(argv[3]); }
        GGPOPlayerHandle h; if(ggpo_add_player(session,&desc,&h)!=GGPO_OK) return 4;
        if(p==player) local=h;
    }
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);
    while(!running && std::chrono::steady_clock::now()<deadline) ggpo_idle(session,1);
    if(!running) return 5;
    int submitted=-1;
    while(state.frame<240 && std::chrono::steady_clock::now()<deadline) {
        ggpo_idle(session,0);
        if(submitted!=state.frame) {
            uint8_t input[12]{};
            input[0]=uint8_t(1u << ((state.frame / 12 + player) % 4));
            input[1]=uint8_t(player * 16);
            if(ggpo_add_local_input(session,local,input,sizeof(input))!=GGPO_OK) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
            submitted=state.frame;
        }
        uint8_t input[24]; int disconnect=0;
        if(ggpo_synchronize_input(session,input,sizeof(input),&disconnect)!=GGPO_OK) continue;
        for(auto b:input) state.checksum=(state.checksum*16777619u)^b;
        ++state.frame;
        ggpo_advance_frame(session);
        std::this_thread::sleep_for(std::chrono::milliseconds(player==2 && state.frame%19==0?25:3));
    }
    // Keep exchanging packets and reconciling the final predictions before comparing.
    for(int i=0;i<400;++i) { ggpo_idle(session,0); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    printf("frame=%d checksum=%u rollbacks=%d\n",state.frame,state.checksum,loads);
    ggpo_close_session(session);
    return state.frame==240 ? 0 : 6;
}
