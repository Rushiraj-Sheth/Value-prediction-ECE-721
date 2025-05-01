#pragma once
#include <bits/stdc++.h>

////////////////////////////// COMPETITION STRUCT ////////////////////////////
struct SHT_struct{
    
    uint64_t tag = 0;   
    uint64_t index = 0;
    int64_t conf_stride = 0;
    uint64_t LRU = 0;
    uint64_t confidence=0;
    bool valid=false;
    uint64_t initial_val=0;
};
//////////////////////////////////////////////////////////////////////////////

struct SVP_struct {
    uint64_t tag = 0;
    uint64_t conf = 0;
    uint64_t retired_value = 0;
    int64_t stride = 0;
    uint64_t instance = 0;    

    uint64_t initial_val=0; //this variavble is for competition part
};



struct VPQ_struct {
   uint64_t PC = 0;
   uint64_t PC_tag = 0;
   uint64_t PC_index = 0;
   int64_t value = 0;
};



//Object of this is (VP) in pipeline.h
class Value_Prediction {
public:

//Can make this in a class for better encapsulation, but for now, keeping it outside.
// making this a vector of dtype what i made.
std::vector<SVP_struct> SVP;
std::vector<VPQ_struct> VPQ;


//You can add more params here...
std::vector<SHT_struct> SHT;

//svp params here
bool enable = false; //This is the enable flag for the VP. This is passed from main.cc (user).
int oracle = 0; //This is the oracle flag. This is passed from main.cc (user).
uint64_t index_len = 0; //This is the length of index bits. This is passed from main.cc (user).
uint64_t tag_len = 0; //This is the length of tag bits used for finding num of SVP entries. This is passed from main.cc (user).
int svp_entries = 0;//THis is used to know where we have added in the SVP Basically the tail
uint64_t MAX_CONF = 0; //This is the max confidence value. This is passed from main.cc (user).



//vpq params here
uint64_t vpq_size = 0; //This is the size of the VPQ. This is passed from main.cc (user).
uint64_t vpq_head = 0;
uint64_t vpq_tail = 0;
bool vpq_hp = false;
bool vpq_tp = false;
bool vpq_temp_tp = false;

//Value_Prediction(int n_enable, int n_index, int n_tag, int n_vpq_size);

//Value_Prediction(uint64_t n_enable, uint64_t n_index, uint64_t n_tag, uint64_t n_vpq_size, uint64_t MAX_CONF);

Value_Prediction(bool n_enable, int oracle, uint64_t n_index, uint64_t n_tag, uint64_t n_vpq_size, uint64_t MAX_CONF);

//////////////////////////// COMPETITION HEADER///////////////////////////////////////////////////
Value_Prediction(bool n_enable, int oracle, uint64_t n_index, uint64_t n_tag,
    uint64_t n_vpq_size, uint64_t MAX_CONF,
    uint64_t len_SHT, uint64_t SHT_MAX_CONF);
//////////////////////////////////////////////////////////////////////////////////////////////////


//Function header here
bool is_VP_eligible(); //This will check if the instruction is eligible for VP or not.
bool is_SVP_search(uint64_t PC); //This will search the SVP for the instruction.
uint64_t get_Tag(uint64_t PC);
uint64_t get_Index(uint64_t PC);

uint64_t prediction(uint64_t PC); //This will predict the value for the instruction.
bool is_SVP_enable();

uint64_t VPQ_allocate(uint64_t PC); //This will allocate the VPQ for the instruction.
bool get_tail_ph(); // return phase bit of vpq tail pointer

bool is_Confident(uint64_t PC); //This will check if the instruction is confident or not.

int is_Oracle();

bool stall_VPQ(uint64_t bundle_size); //This will check if the VPQ should slall or not, based on the size of the bundle.

void train_SVP(uint64_t PC,uint64_t C_value, bool confident, uint64_t vpq_entry); // for training the SVP at retirement

void VP_Debug_Prints(FILE* file);
void VP_stats(FILE* file);

void svp_debug_prints(FILE* stats_log);
void vpq_debug_prints(FILE* stats_log);
void svp_squash();
void VPQ_recovery(uint64_t entry, bool entry_ph);

void print_vpq();

///////////////////////// COMPETITION HEADER ////////////////////////////////////////////////
uint64_t SHT_MAX_CONF=0; //stride history table
uint64_t SHT_size = 0;


uint64_t Competition_prediction(uint64_t PC);
int get_sht_pos(uint64_t PC);
void Competition_update_SHT(uint64_t PC);
void update_LRU(int lru_pos,int sht_pos);
bool search_SHT(uint64_t PC);
///////////////////////////////////////////////////////////////////////////////////////////////

};
