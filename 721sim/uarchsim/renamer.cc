#include<renamer.h>

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs, uint64_t n_branches, uint64_t n_active)
{
    //asserting phy_reg > log_reg
    assert(n_phys_regs > n_log_regs);

    //asserting no. of branches
    assert(n_branches>=1 && n_branches<=64);

    //asserting active list size
    assert(n_active > 0);

    AMT.resize(n_log_regs);
    RMT.resize(n_log_regs);

    int fl_size = n_phys_regs - n_log_regs;
    FL.resize(fl_size); // PRF Size - committed registers

    AL.resize(n_active);
    PRF.resize(n_phys_regs);
    RDY_BIT.resize(n_phys_regs); 

    //pointers
    fl_head=0; fl_tail=0; fl_h_phase=0; fl_t_phase=1; // h_phase != t_pahse --> Full 
    al_head=0; al_tail=0; al_h_phase=0; al_t_phase=0; // h_phase=t_phase --> empty

    //initalising data structures
    //AMT and RMT
    for(int i=0; i<AMT.size();i++){
        AMT[i].committed_phy_reg_mapping = i;
        RMT[i].phy_reg_mapping = i;
    }

    // PRF and READY_BITS
    for(int i=0;i<PRF.size();i++){
        //PRF[i].value = -1;
        RDY_BIT[i].rdy = 1;
    }

    //Free list
    free_list_entries      = FL.size(); // entire list available for use
    int free_phy_reg_entry = n_log_regs;
    for(int i=0;i<FL.size(); i++){
        FL[i].phy_reg_num = free_phy_reg_entry;
        free_phy_reg_entry++;
    }

    //Active list
    active_list_empty_entries   = AL.size(); //entire list avail. for use
    for(int i=0;i<AL.size();i++){
        AL[i].amo_flg           = 0;
        AL[i].br_flg            = 0;
        AL[i].br_mispred_bit    = 0;
        AL[i].complete_bit      = 0;
        AL[i].csr_flg           = 0;
        AL[i].dest_flag         = 1; // by default all inst have dest
        AL[i].execp_bit         = 0;
        AL[i].load_flg          = 0;
        AL[i].load_viol_bit     = 0;
        AL[i].logical_reg_num   = 0;
        AL[i].pc                = 0;
        AL[i].phy_reg_num       = 0;
    }

    BR_CHK.resize(n_branches);
    GBM = 0;
    for(int i=0;i<BR_CHK.size();i++){
        BR_CHK[i].SMT.resize(n_log_regs);
        BR_CHK[i].gbm_check = GBM;
    }


//    cout<<"al size"<<AL.size()<<endl;


}

bool renamer::is_FL_empty(){
    if( (fl_head == fl_tail) && (fl_h_phase==fl_t_phase) ){
        //empty
        return true;
    }
    
    return false;
}

bool renamer::is_FL_full(){
    if( (fl_head == fl_tail) && (fl_h_phase!=fl_t_phase) ){
        //full
        return true;
    }

    return false;
}

bool renamer::is_AL_empty(){
    if( (al_head==al_tail) && (al_h_phase == al_t_phase) ){
        return true;
    }

    return false;
}

bool renamer::is_AL_full(){
    if( (al_head==al_tail) && (al_h_phase != al_t_phase) ){
        return true;
    }

    return false;
}

int renamer::avail_FL_entries(){

    int free_entries=0;
    
    if(is_FL_full()){
        free_entries = FL.size();
    }
    else{
        
        if(is_FL_empty()){
            free_entries = 0;
        }
        else{
            
            if(fl_head<fl_tail){
                free_entries = fl_tail - fl_head;
            }
            else{
                free_entries = FL.size()-(fl_head - fl_tail);
            }
        }

    }
    //cout<<"FL free entries: "<<free_entries<<endl;
    return free_entries;
}


int renamer::avail_AL_entries(){
    int free_entries = 0;
    if( is_AL_full() ){
        free_entries = 0;
    }
    else{
        
        if(is_AL_empty()){
            free_entries = AL.size();
        }
        else{
            
            if(al_head<al_tail){
                free_entries = AL.size() - (al_tail - al_head);
            }
            else{
                free_entries = al_head - al_tail;
            }
        }

    }
   // cout<<"al free entries: "<<free_entries<<endl;
    return free_entries;    
}



bool renamer::stall_reg(uint64_t bundle_dst){
    //check available free reg in free list
    int free_phy_reg = avail_FL_entries();
    if( (free_phy_reg) >= bundle_dst){
        return false;
    }

    return true;
}


bool renamer::stall_branch(uint64_t bundle_branch)
{
    int temp_gbm = GBM;
    int no_of_occupied_branches=0;
    int mask = 1;

    for(int i=0;i<BR_CHK.size();i++){
        if(1 == (temp_gbm&mask)){
            no_of_occupied_branches++;
        }
        temp_gbm = temp_gbm>>1;
    }

    if( (BR_CHK.size() - no_of_occupied_branches) >= bundle_branch){
        return false;
    }

    return true;
}

uint64_t renamer::get_branch_mask(){
    return GBM;
}

uint64_t renamer::rename_rsrc(uint64_t log_reg){
    
    return RMT[log_reg].phy_reg_mapping;
}


uint64_t renamer::rename_rdst(uint64_t log_reg){
    //rename dest
    int new_dest = FL[fl_head].phy_reg_num;
    fl_head = (fl_head+1)%(FL.size());
    if(fl_head == 0)fl_h_phase = !fl_h_phase;
    free_list_entries--;
    
    //update RMT
    RMT[log_reg].phy_reg_mapping = new_dest;

    return new_dest;
}


bool renamer::stall_dispatch(uint64_t bundle_inst){
    
    int free_al_entries = avail_AL_entries();
    if(free_al_entries >= bundle_inst){
        return false;
    }

    return true;
}

uint64_t renamer::dispatch_inst(bool dest_valid, uint64_t log_reg, uint64_t phys_reg, 
    bool load, bool store, bool branch,
    bool amo,
    bool csr,
    uint64_t PC)
{

    //assert active list is not full
    assert(avail_AL_entries()>0);

    AL[al_tail].dest_flag   = dest_valid;
    AL[al_tail].phy_reg_num = phys_reg;
    AL[al_tail].logical_reg_num = log_reg;    
    AL[al_tail].load_flg    = load;
    AL[al_tail].str_flg     = store;
    AL[al_tail].br_flg      = branch;
    AL[al_tail].amo_flg     = amo;
    AL[al_tail].csr_flg     = csr;
    AL[al_tail].pc          = PC;
    // reset the mis bits
    AL[al_tail].br_mispred_bit = 0;
    AL[al_tail].complete_bit   = 0;
    AL[al_tail].execp_bit      = 0;
    AL[al_tail].load_viol_bit  = 0;
    AL[al_tail].val_mispred_bit = 0;

    //inc tail
    int temp_tail = al_tail;
    al_tail = (al_tail + 1)%(AL.size());
    if(0 == al_tail)al_t_phase = !al_t_phase;
    active_list_empty_entries--; //decrease total free entries

    return temp_tail;

}


bool renamer::is_ready(uint64_t phys_reg){

    return (RDY_BIT[phys_reg].rdy==1);
}


void renamer::clear_ready(uint64_t phys_reg){
    RDY_BIT[phys_reg].rdy = 0;
}


uint64_t renamer::read(uint64_t phys_reg){
    return PRF[phys_reg].value;
}

void renamer::set_ready(uint64_t phys_reg){
    RDY_BIT[phys_reg].rdy = 1;
}

void renamer::write(uint64_t phys_reg, uint64_t value){
    PRF[phys_reg].value = value;
}

void renamer::set_complete(uint64_t AL_index){
    AL[AL_index].complete_bit = 1;
}

bool renamer::precommit(bool &completed,
    bool &exception, bool &load_viol, bool &br_misp, bool &val_misp,
bool &load, bool &store, bool &branch, bool &amo, bool &csr,
uint64_t &PC)
{
    if(true == is_AL_empty()){
        return false;
    }

    completed = AL[al_head].complete_bit;
    exception = AL[al_head].execp_bit;
    load_viol = AL[al_head].load_viol_bit;
    br_misp   = AL[al_head].br_mispred_bit;
    val_misp  = AL[al_head].val_mispred_bit;
    load      = AL[al_head].load_flg;
    store     = AL[al_head].str_flg;
    branch    = AL[al_head].br_flg;
    amo       = AL[al_head].amo_flg;
    csr       = AL[al_head].csr_flg;
    PC        = AL[al_head].pc;
    
    return true;
}


void renamer::commit(){
    //assert(active_list_empty_entries < AL.size());
    assert(false == is_AL_empty());
    assert(AL[al_head].complete_bit == 1);
    assert(AL[al_head].execp_bit == 0);
    assert(AL[al_head].load_viol_bit == 0);

    //free the current committed phy reg from AMT
    //only if dest flag is valid
    if(1 == AL[al_head].dest_flag){
        FL[fl_tail].phy_reg_num = AMT[AL[al_head].logical_reg_num].committed_phy_reg_mapping;
        fl_tail = (fl_tail+1)%(FL.size());
        if(0 == fl_tail)fl_t_phase = !fl_t_phase;
        free_list_entries++;

        //commit the current mapping from AL to AMT
        AMT[AL[al_head].logical_reg_num].committed_phy_reg_mapping = AL[al_head].phy_reg_num;

    }
    AL[al_head].complete_bit=0;
    AL[al_head].br_flg      =0;
    al_head = (al_head+1)%(AL.size());
    if(0 == al_head)al_h_phase = !al_h_phase;
    active_list_empty_entries++; //instr. removerd from AL

}


void renamer::squash(){
    //restoring RMT
    for(int i=0;i<RMT.size();i++){
        RMT[i].phy_reg_mapping = AMT[i].committed_phy_reg_mapping;
    }

    //restoring free list
    fl_head=fl_tail;
    fl_h_phase = !fl_t_phase;
    free_list_entries = FL.size();

    //restoring AL
    al_tail = al_head;
    al_t_phase = al_h_phase;
    active_list_empty_entries = AL.size();

    // free all outstanding branch checkpoints
    GBM = 0;
}

void renamer::set_exception(uint64_t AL_index){
    AL[AL_index].execp_bit = 1;
}

void renamer::set_load_violation(uint64_t AL_index){
    AL[AL_index].load_viol_bit = 1;
}

void renamer::set_branch_misprediction(uint64_t AL_index){
    AL[AL_index].br_mispred_bit = 1;
}

void renamer::set_value_misprediction(uint64_t AL_index){
    AL[AL_index].val_mispred_bit = 1;
}

bool renamer::get_exception(uint64_t AL_index){
    return (1 == AL[AL_index].execp_bit);
}

uint64_t renamer::set_GBM_bit(){
    int pos = -1;
    int temp_gbm = GBM, mask=1;
    for(int i=0;i<BR_CHK.size(); i++){
        int gbm_bit = (temp_gbm & mask);
        if(0 == gbm_bit){
            pos = i;
            break;
        }
        temp_gbm = temp_gbm>>1;
    }

    assert(pos>-1);

    int set_bit_mask = 1 << (pos);
    //Now, checkpoint GBM before incrementing it.
    BR_CHK[pos].gbm_check = GBM;
    //increment the GBM.
    GBM = (set_bit_mask | GBM);
    return pos;
    

}

uint64_t renamer::checkpoint(){
    assert( false == stall_branch(1) );

    uint64_t branch_id = set_GBM_bit();
    for(int i=0;i<RMT.size();i++){
        BR_CHK[branch_id].SMT[i].phy_reg_mapping = RMT[i].phy_reg_mapping;
    }
    
    BR_CHK[branch_id].head     = fl_head;
    BR_CHK[branch_id].head_ph  = fl_h_phase;
    //GBM checked in set_GBM_bit function
    return branch_id;    
}

void renamer::clear_GBM_bit(uint64_t branch_id){
    uint64_t pos=branch_id;
    uint64_t mask = ~(1<<(pos));
    GBM = GBM&mask;
}

uint64_t renamer::clear_bit(uint64_t branch_id, uint64_t chk_gbm){
    uint64_t pos=branch_id;
    uint64_t mask = ~(1<<(pos));
    chk_gbm = chk_gbm&mask;
    return chk_gbm;
}

void renamer::resolve(uint64_t AL_index,uint64_t branch_ID,bool correct)
{

    if(true == correct){
        clear_GBM_bit(branch_ID);
        for(int i=0;i<BR_CHK.size();i++){
            uint64_t new_gbm = clear_bit(branch_ID,BR_CHK[i].gbm_check);
            BR_CHK[i].gbm_check = new_gbm;
        }


        //clear br checkpoint from checkpoints. This is redundant bcz
        // the GBM's bit is cleared. The checkpoints will be overwritten in the future.
        // BR_CHK[branch_ID].gbm_check = 0;
        // BR_CHK[branch_ID].head      = 0;
        // BR_CHK[branch_ID].head_ph   = 0;
        // for(int i=0;i<RMT.size();i++){
        //     BR_CHK[branch_ID].SMT[i].phy_reg_mapping = 0;
        // }
    }
    else{

        //restore gbm from branch checkpoint
        uint64_t restored_gbm = BR_CHK[branch_ID].gbm_check;
        GBM                   = restored_gbm; //non-incremented checkpointed GBM restored
        
        //restore RMT
        for(int i=0;i<RMT.size();i++){
            RMT[i].phy_reg_mapping = BR_CHK[branch_ID].SMT[i].phy_reg_mapping;
        }

        //restore FL pointers
        fl_head     = BR_CHK[branch_ID].head;
        fl_h_phase  = BR_CHK[branch_ID].head_ph;

        //restoring AL tail
        al_tail = (AL_index+1)%(AL.size());
        if(al_head < al_tail){
            al_t_phase = al_h_phase;
        }
        if(al_head > al_tail){
            al_t_phase = !al_h_phase;
        }

        //condition of T==H is redundant bcz of T<H condition
        // if( (AL_index<al_head) && (al_tail == al_head) ){
        //     al_t_phase = !al_h_phase;
        // }

        //the br is resolved but mispredicted. so also need to clear its own checpoints
        // clearing the branch's own checkpoints is redundant bcz
        // its own GBM bit is cleared. So, they will be ovwrwritten in future

        // BR_CHK[branch_ID].gbm_check = 0;
        // BR_CHK[branch_ID].head      = 0;
        // BR_CHK[branch_ID].head_ph   = 0;
        // for(int i=0;i<RMT.size();i++){
        //     BR_CHK[branch_ID].SMT[i].phy_reg_mapping = 0;
        // }

    }
}

///////////////////////////////////////////////////////////////////////////////
