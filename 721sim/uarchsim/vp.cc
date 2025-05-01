#include "vp.h"
#include "pipeline.h"

//Constructor
Value_Prediction::Value_Prediction(bool n_enable, int oracle, uint64_t n_index, uint64_t n_tag, uint64_t n_vpq_size, uint64_t MAX_CONF)
   //: enable(n_enable), index(n_index), svp_entries(2 * n_index), vpq_size(n_vpq_size), 
      //vpq_head(0), vpq_tail(0), vpq_hp(false), vpq_tp(false)
{
   //Initializing the values coming from main
   enable = n_enable;
   this->oracle = oracle; // This is the oracle flag. This is passed from main.cc (user).
   index_len = n_index;
   tag_len = n_tag; // This is the tag used for finding num of SVP entries. This is passed from main.cc (user).
   svp_entries = pow(2,n_index); // This is the number of SVP entries. This is passed from main.cc (user).
   vpq_size = n_vpq_size; // This is the size of the VPQ.
   this->MAX_CONF = MAX_CONF; // This is the max confidence value. This is passed from main.cc (user).

   vpq_head = 0; // This is the head of the VPQ.
   vpq_tail = 0; // This is the tail of the VPQ.
   vpq_hp = false; // This is the head pointer of the VPQ.
   vpq_tp = false; // This is the tail pointer of the VPQ.

   //std::cout << "CONFIG: " << n_index << n_tag << svp_entries << vpq_size << std::endl;


   // Constructor implementation allocating the sizes
   SVP.resize(svp_entries);
   VPQ.resize(vpq_size);
}

Value_Prediction::Value_Prediction(bool n_enable, int oracle, uint64_t n_index, uint64_t n_tag,
                                    uint64_t n_vpq_size, uint64_t MAX_CONF,
                                    uint64_t len_SHT, uint64_t SHT_MAX_CONF){

   //Initializing the values coming from main
   enable = n_enable;
   this->oracle = oracle; // This is the oracle flag. This is passed from main.cc (user).
   index_len = n_index;
   tag_len = n_tag; // This is the tag used for finding num of SVP entries. This is passed from main.cc (user).
   svp_entries = pow(2,n_index); // This is the number of SVP entries. This is passed from main.cc (user).
   vpq_size = n_vpq_size; // This is the size of the VPQ.
   
   this->MAX_CONF = MAX_CONF; // This is the max confidence value. This is passed from main.cc (user).
   this->SHT_MAX_CONF = SHT_MAX_CONF;
   SHT_size = pow(2,len_SHT);

   vpq_head = 0; // This is the head of the VPQ.
   vpq_tail = 0; // This is the tail of the VPQ.
   vpq_hp = false; // This is the head pointer of the VPQ.
   vpq_tp = false; // This is the tail pointer of the VPQ.
                                    
   //std::cout << "CONFIG: " << n_index << n_tag << svp_entries << vpq_size << std::endl;


   // Constructor implementation allocating the sizes
   SVP.resize(svp_entries);
   VPQ.resize(vpq_size);


   SHT.resize(SHT_size);

   for(int j=0;j<SHT_size;j++){
      SHT[j].LRU = j;
      SHT[j].valid = false;
      SHT[j].confidence = 0;
   }


}
//////////////////////////////////////////////////////////////////////////////////

uint64_t Value_Prediction::get_Tag(uint64_t PC){
   return (PC&((1<<(index_len+tag_len+2))-1))>>(index_len+2); // This is the tag of the SVP.
}

uint64_t Value_Prediction::get_Index(uint64_t PC){
   return (PC&((1<<(index_len+2))-1))>>2; // This is the index of the SVP.
}


//////////////////////////////////////////////////////////////////////////////////
bool Value_Prediction::is_SVP_search(uint64_t PC) {
   bool is_hit;
   //assert(tag_len>0);
   uint64_t vpq_index = get_Index(PC); // This is the index of the SVP.
   uint64_t vpq_tag = get_Tag(PC); // This is the tag of the SVP.

   //Check if the tag is equal to the tag of the SVP.
   if(SVP[vpq_index].tag == vpq_tag ){
      is_hit = true;
   }
   else{
      is_hit = false;
   }

   return is_hit;
}
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
uint64_t Value_Prediction::prediction(uint64_t PC) {
   uint64_t t_pred_val = 0;

   uint64_t vpq_index = get_Index(PC); // This is the index of the SVP.
   uint64_t vpq_tag = get_Tag(PC); // This is the tag of the SVP.


   SVP[vpq_index].instance++; // Increment the instance of the SVP.
   t_pred_val = SVP[vpq_index].retired_value + (SVP[vpq_index].stride * SVP[vpq_index].instance); // This is the prediction value.


   return t_pred_val;
}
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
bool Value_Prediction::is_SVP_enable() {
   return enable;
}
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
uint64_t Value_Prediction::VPQ_allocate(uint64_t PC) {
   uint64_t t_vpq_tail = vpq_tail;
   vpq_temp_tp = vpq_tp;
   VPQ[t_vpq_tail].PC = PC; // This is the PC of the VPQ.
   VPQ[t_vpq_tail].PC_tag = get_Tag(PC);
   VPQ[t_vpq_tail].PC_index = get_Index(PC); 

   vpq_tail++;
   //cout<<"tail"<<vpq_tail<<"vpqindex"<<VPQ[t_vpq_tail].PC_index<<endl;
   if(vpq_tail == vpq_size) { // If the tail of the VPQ is equal to the size of the VPQ, then set it to 0.
      vpq_tail = 0;
      vpq_tp = !vpq_tp;
   }


   return t_vpq_tail; // Return the tail of the VPQ.
}
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
bool Value_Prediction::is_Confident(uint64_t PC) {
  
   bool t_is_confident = false;

   uint64_t vpq_index = get_Index(PC); // This is the index of the SVP.
   uint64_t vpq_tag = get_Tag(PC); // This is the tag of the SVP.

   //Already checked if the tag is equal to the tag of the SVP (in rename.cc). But Rushi will call from so keeping it here.
   if( SVP[vpq_index].conf == MAX_CONF  ){ // && (SVP[vpq_index].tag == vpq_tag)
      
      t_is_confident = true;
   }
   else{
      t_is_confident = false;
   }

   return t_is_confident;
}
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
int Value_Prediction::is_Oracle() {

   return oracle;
}
//////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////
bool Value_Prediction::stall_VPQ(uint64_t num_vp_instr) {//This will check if the VPQ should slall or not, based on the size of the bundle.
   
   int avail_entries = 0;

   // vpq empty
   if( (vpq_head == vpq_tail) && (vpq_hp == vpq_tp) ){
      //empty => dont stall
      return false;
   }

   //full
   if( (vpq_head == vpq_tail) && (vpq_hp != vpq_tp) ){
      // full -> stall
      return true;
   }

   if(vpq_head < vpq_tail){
      avail_entries = vpq_size - (vpq_tail - vpq_head);
   }

   if(vpq_head > vpq_tail ){
      avail_entries = vpq_head - vpq_tail;
   }

   assert(avail_entries>0);

   if(avail_entries >= num_vp_instr){
      // we have space for new instrs -> dont stall
      return false;
   }
   else{
      // we dont have space -> stall
      return true;
   }

}
//////////////////////////////////////////////////////////////////////////////////

bool Value_Prediction::get_tail_ph(){
   return vpq_temp_tp;
}

//////////////////////////////////////////////////////////////////////////////////
/// train the SVP
void Value_Prediction::train_SVP(uint64_t PC, uint64_t C_value, bool confident, uint64_t vpq_entry ){

   if( C_value != VPQ[vpq_head].value ){
   cout<<"ASSERTION: pc " << std::hex << PC << ", vpq_entry " << vpq_entry <<  ", vpq_head: " << vpq_head <<endl;
   }

   uint64_t temp_vpq_head;
   uint64_t PC_tag = get_Tag(PC);
   uint64_t PC_index = get_Index(PC);

   assert(vpq_head == vpq_entry);
   assert(VPQ[vpq_head].value == C_value);
   assert(VPQ[vpq_head].PC == PC);
   assert(VPQ[vpq_head].PC_index == PC_index);
   assert(VPQ[vpq_head].PC_tag == PC_tag);

   temp_vpq_head = vpq_head;
   vpq_head = (vpq_head + 1) % (vpq_size);
   if(vpq_head == 0){
      vpq_hp = !vpq_hp;
   }
   
   // 1: search the SVP
   // if hit:
   bool hit = is_SVP_search(PC);
   //uint64_t svp_id = VPQ[temp_vpq_head].PC_index;
   //assert(svp_id == PC_index);

   if(hit){
      int64_t new_stride = C_value - SVP[PC_index].retired_value;
      if(new_stride == SVP[PC_index].stride){

         SVP[PC_index].conf++;
         if(SVP[PC_index].conf >= MAX_CONF){
            SVP[PC_index].conf = MAX_CONF;
         }
      }
      else{
         // stride mismatch
         // reset the confidence
         SVP[PC_index].conf = 0;
         // reset the stride
         SVP[PC_index].stride = new_stride;
      }

      SVP[PC_index].retired_value = C_value;
      assert(SVP[PC_index].instance > 0);
      SVP[PC_index].instance-=1;
      
      // if(PC_index==18){
      //    cout<<"HIT----\n"<<"SVP: "<<SVP[svp_id].instance<<endl;
      //    print_vpq();
      //    cout<<"svp tag: "<<SVP[18].tag<<" svp conf:"<<SVP[18].conf<<" svp RV:"<<SVP[18].retired_value<<" svp stride:"<<SVP[18].stride<<" svp instance:"<<SVP[18].instance<<endl;
      //    cout<<"-------------"<<endl;
      // }
      
   }
   else{
      // if miss:

      /////////// COMPETITION PART /////////////////////
         if(COMPETE){
            if( (SVP[PC_index].conf>0) && (SVP[PC_index].conf<MAX_CONF) ){
               //store this in SHT
               Competition_update_SHT(PC);
            }
         }
      ///////////////////////////////////////////////

      if(COMPETE){
         SVP[PC_index].initial_val = C_value;
      }
      SVP[PC_index].tag = PC_tag;
      SVP[PC_index].conf = 0;
      SVP[PC_index].retired_value = C_value;
      SVP[PC_index].stride = C_value;
      SVP[PC_index].instance = 0; //setting below by counting
      

      uint64_t temp = vpq_head;
      bool temp_hp = vpq_hp;

      while( temp != vpq_tail || temp_hp!=vpq_tp){
         if( PC_tag == VPQ[temp].PC_tag && PC_index==VPQ[temp].PC_index ) //   PC == VPQ[temp].PC 
         {
            SVP[PC_index].instance += 1;
         }
         temp = (temp+1)%(vpq_size);
         if(temp==0){
            temp_hp = !temp_hp;
         }
      } 


      // if(PC_index==18){
      //    cout<<"MISS----\n"<<"SVP: "<<SVP[svp_id].instance<<endl;
      //    print_vpq();
      //    cout<<"svp tag: "<<SVP[18].tag<<" svp conf:"<<SVP[18].conf<<" svp RV:"<<SVP[18].retired_value<<" svp stride:"<<SVP[18].stride<<" svp instance:"<<SVP[18].instance<<endl;
      //    cout<<"MISS----\n"<<endl;
      // }
        
   }


   // VPQ[temp_vpq_head].PC = 0;
   // VPQ[temp_vpq_head].PC_tag = 0;
   // VPQ[temp_vpq_head].PC_index = 0;
}
//////////////////////////////////////////////////////////////////////////////////

void Value_Prediction::svp_squash(){
   //reset VPQ
   uint64_t vpq_pc;
   uint64_t vpq_pc_index;

    //cout<<"vpq_squash"<<endl;

   while( !(vpq_tail==vpq_head && vpq_tp==vpq_hp) ){
      //vpq not empty
      //assert(!(vpq_tail==vpq_head && vpq_tp==vpq_hp));
      //cout<<"vpq head "<<vpq_head<<" vpq_hp: "<<vpq_hp<<" vpq_tail: "<<vpq_tail<<" vpq_tp: "<<vpq_tp<<" VPQ_ENTRY: "<<VPQ_ENTRY<<" VPQ_ENTRY_PH"<<VPQ_ENTRY_PH<<endl;
      if(vpq_tail==0){
         vpq_tail=vpq_size-1;
         vpq_tp=!vpq_tp;
      }
      else{
         vpq_tail--;
      }

      //cout<<"vpq_head:"<<vpq_head<<" vpq_tail:"<<vpq_tail<<endl;

      vpq_pc = VPQ[vpq_tail].PC;
      vpq_pc_index = VPQ[vpq_tail].PC_index;

      if( is_SVP_search(vpq_pc) && SVP[vpq_pc_index].instance > 0){
         assert(SVP[vpq_pc_index].instance>0);
         SVP[vpq_pc_index].instance-=1;
      }

      //SVP[vpq_pc_index].instance=0;
   }

   //now vpq h == vpq t
   assert(vpq_head==vpq_tail);
   assert(vpq_hp==vpq_tp);

   // for(int i=0;i<svp_entries;i++){
   //    SVP[i].instance = 0;
   // }

  // print_vpq();
   // VPQ_ENTRY = vpq_tail;
   // VPQ_ENTRY_PH = vpq_tp;
   // VALID_VPQ_ENTRY = false;

   // cout<<"#############################\n";
   // cout<<"vpq head "<<vpq_head<<" vpq_hp: "<<vpq_hp<<" vpq_tail: "<<vpq_tail<<" vpq_tp: "<<vpq_tp<<" VPQ_ENTRY: "<<VPQ_ENTRY<<" VPQ_ENTRY_PH"<<VPQ_ENTRY_PH<<endl;
   // cout<<"#############################\n";
}
//////////////////////////////////////////////////////////////////////////////////

void Value_Prediction::print_vpq(){
   uint64_t temp=vpq_head;
   uint64_t temp_ph=vpq_hp;

   cout<<"###################### VPQ ########################"<<endl;
   //int i=0;i<vpq_size;i++ 
   while(!(temp==vpq_tail && temp_ph==vpq_tp) ){
      cout<<" head: "<<vpq_head<<" hp: "<<vpq_hp<<" tail: "<<vpq_tail<<" tp: "<<vpq_tp<<" PC: "<<VPQ[temp].PC<<" TAG: "<<VPQ[temp].PC_tag<<" Indx: "<<VPQ[temp].PC_index<<endl;
      
      temp=(temp+1)%vpq_size;
      if(temp==0){
         temp_ph=!temp_ph;
      }
   }
   cout<<"xxxxxxxxxxxxxxxxxxxxxxxxxx VPQ xxxxxxxxxxxxxxxxxxxxxxxxx"<<endl;
}

/////////////////////////////////////////////////////////////////////////////////

void Value_Prediction::VPQ_recovery(uint64_t entry, bool entry_ph){

   // empty
   // if( vpq_head==vpq_tail && vpq_hp==vpq_tp ){
   //    return;
   // } 

   /// rollback the instance counters
   uint64_t vpq_pc;
   uint64_t vpq_pc_index;
   //cout<<"VPQ RECOVERY"<<endl;
   while( !(vpq_tail==entry && vpq_tp==entry_ph) ) 
   {
      //assert(!(vpq_tail==entry && vpq_tp==entry_ph));

      if(vpq_tail==0){
         vpq_tail=vpq_size-1;
         vpq_tp = !vpq_tp;
      }
      else{
         vpq_tail--;
      }
      //cout<<"vpq_head:"<<vpq_head<<", hp:"<<vpq_hp<<" vpq_tail:"<<vpq_tail<<", tp:"<<vpq_tp<<", entry:"<<entry<<", entry_ph:"<<entry_ph<<endl;

      vpq_pc = VPQ[vpq_tail].PC;
      vpq_pc_index = VPQ[vpq_tail].PC_index;

      if(is_SVP_search(vpq_pc)){
         assert(SVP[vpq_pc_index].instance > 0);
         SVP[vpq_pc_index].instance-=1;
      }
   }

   // vpq_tail = entry;
   // vpq_tp = entry_ph;

   //print_vpq();
   // cout<<"#############################\n";
   // cout<<"vpq head "<<vpq_head<<" vpq_hp: "<<vpq_hp<<" vpq_tail: "<<vpq_tail<<" vpq_tp: "<<vpq_tp<<" VPQ_ENTRY: "<<VPQ_ENTRY<<" VPQ_ENTRY_PH"<<VPQ_ENTRY_PH<<endl;
   // cout<<"#############################\n";
}

//////////////////////////////////////////////////////////////////////////////////


////////////////////////////// COMPETITION FUNCTIONS /////////////////////////////
bool Value_Prediction::search_SHT(uint64_t PC){


   // sort the SHT based on LRU
   std::sort(SHT.begin(), SHT.end(), [](const SHT_struct& a, const SHT_struct& b) {
      return a.LRU < b.LRU; // Sort in ascending order
   });

   int sht_pos = get_sht_pos(PC);
   if(sht_pos == -1){
      return false;
   } 
   
   return true;
}


int Value_Prediction::get_sht_pos(uint64_t PC){

   int pos=-1;
   uint64_t PC_tag = get_Tag(PC);
   uint64_t PC_index = get_Index(PC);

   for(int i=0;i<SHT_size;i++){
      if( PC_tag == SHT[i].tag && PC_index==SHT[i].index && SHT[i].valid){
         pos = i;
         break;
      }
   }

   return pos;
}


uint64_t Value_Prediction::Competition_prediction(uint64_t PC) {
   
   uint64_t t_pred_val = 0;
   uint64_t base_prediction = 0;
   uint64_t sht_prediction = 0;

   uint64_t PC_index = get_Index(PC); // This is the index of the SVP.
   uint64_t PC_tag = get_Tag(PC); // This is the tag of the SVP.

   //vpq walk
   uint64_t temp = vpq_head;
   bool temp_hp = vpq_hp;
   int sht_pos = get_sht_pos(PC);
   assert(sht_pos!=-1);

   uint64_t sht_instance = 0;


   while( !(temp==vpq_tail && temp_hp==vpq_hp) ){
      if(VPQ[temp].PC_index == SHT[sht_pos].index && VPQ[temp].PC_tag==SHT[sht_pos].tag){
         sht_instance += 1;
      }

      temp = (temp+1)%(vpq_size);
      if(temp==0){
         temp_hp = !temp_hp;
      }
   }

   
   sht_prediction = SHT[sht_pos].initial_val + (sht_instance * SHT[sht_pos].conf_stride);

   return sht_prediction;

}

//////////////////////////////////////////////////////////////////////////////////
/// Update SHT
//void Value_Prediction::Competetion_update_SHT() {
   //This is Assuming that we have already trainted the SVP only so we already have the new stride..... not once not twice but till CONF_MAX times.
//This code is to update the SHT
//No dont loop for all the SVP. only one which will have a tag hit/PC hit

//We need to look into 2 things (First) Add a new stride to the SHT if not full
//                              (Second) Replace the Least Recently used stride with the new stride in SVP who got max conf.

//Step 1: Check if the stride of that particular index is at max_conf.
//Step 2: if confidence is max then check in the SHT to find the matching stride.
//Step 3: if we found a matching stride inside the SHT then increment the instance_history of that stride.

//Step 4: if we did not find the matching stride in the SHT then we would need to add a new stride.
//Step 5: Check if the SHT is full of not.

//Step 6: if not full then we will add the new stride to the empty slot of SHT.

//Step 7: Call a update LRU function that will updated the LRU of SHT.
//Step 8: if SHT is full we need to replace Least recently used stride.
//Step 9: Callt the update LRU function.

void Value_Prediction::Competition_update_SHT(uint64_t PC) {

   uint64_t PC_index = get_Index(PC);
   uint64_t PC_tag  = get_Tag(PC);

   assert(SVP[PC_index].conf>0);
   assert(SVP[PC_index].conf<MAX_CONF);

   bool found = false;
   // Step: look for an empty slot
   for (int j = 0; j < SHT_size; j++) {
      if( !SHT[j].valid ){
         found = true;

         // install info from SVP
         SHT[j].valid = true;
         SHT[j].index = PC_index;
         SHT[j].tag   = SVP[PC_index].tag;
         SHT[j].initial_val = SVP[PC_index].initial_val;
         SHT[j].conf_stride = SVP[PC_index].stride;
         update_LRU( SHT[j].LRU, j );
         break;
      }
   }

      // replacement
   if (!found) {
      
      bool added = false;

      // sort the SHT in ascending order of LRU
      std::sort(SHT.begin(), SHT.end(), [](const SHT_struct& a, const SHT_struct& b) {
         return a.LRU < b.LRU; // Sort in ascending order
      });

      // now replace the last entry
      SHT[SHT_size-1].valid = true;
      SHT[SHT_size-1].index = PC_index;
      SHT[SHT_size-1].tag   = SVP[PC_index].tag;
      SHT[SHT_size-1].initial_val = SVP[PC_index].initial_val;
      SHT[SHT_size-1].conf_stride = SVP[PC_index].stride;
      update_LRU( SHT[SHT_size-1].LRU, SHT_size-1 );
   }
}
//////////////////////////////////////////////////////////////////////////////////

void Value_Prediction::update_LRU(int curr_lru, int sht_pos){

   for(int i=0;i<SHT_size;i++){
      if(SHT[i].LRU < curr_lru){
         assert(SHT[i].valid);
         SHT[i].LRU++;
      }
   }

   SHT[sht_pos].LRU = 0;
}

//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////

////////////////////////// PRINT FUNCS BELOW /////////////////////////////////////
void Value_Prediction::VP_Debug_Prints(FILE* file){

   fprintf(file, "\n=== VALUE PREDICTOR ============================================================\n");

   //VP-eligible configuration: 
   fprintf(file, "\nVP-eligible configuration:\n");
   fprintf(file, "   predINTALU = %d \n", predINTALU);
   fprintf(file, "   predFPALU  = %d \n", predFPALU);
   fprintf(file, "   predLOAD   = %d \n", predLOAD);

   fprintf(file, "\nVALUE PREDICTOR = ");

   if(PERF_VP)
   {
       fprintf(file, "perfect\n");
   }
   else if(enable)
   {
       fprintf(file, "stride (Project 4 spec. implementation)\n");
       fprintf(file, "   VPQsize             = %lu \n", vpq_size);
       fprintf(file, "   oracleconf          = %d ", oracle);
       if(oracle)
           fprintf(file, "(oracle confidence)\n");
       else
           fprintf(file, "(real confidence)\n");
       fprintf(file, "   # index bits        = %lu \n", index_len);
       fprintf(file, "   # tag bits          = %lu \n", tag_len);
       fprintf(file, "   confmax             = %lu \n", MAX_CONF);
       
   }

   fprintf(file, "\nCOST ACCOUNTING\n");
   if(PERF_VP){
      fprintf(file,"Impossible.\n");
   }
   else{
      uint64_t SVP_bits = tag_len + (uint64_t)ceil(log2((double)(MAX_CONF+1))) + 
                        (sizeof(SVP[0].retired_value) * 8) + 
                        (sizeof(SVP[0].stride) * 8) + 
                        (uint64_t)ceil(log2((double)vpq_size)
                     );

      fprintf(file, "\tOne SVP entry:\n");
      fprintf(file, "\t\ttag              :%4lu bits  // num_tag_bits\n", tag_len);
      fprintf(file, "\t\tconf             :%4lu bits  // formula: (uint64_t)ceil(log2((double)(confmax+1)))\n", (uint64_t)ceil(log2((double)(MAX_CONF+1))));
      fprintf(file, "\t\tretired_value    :%4lu bits  // RISCV64 integer size.\n", (sizeof(SVP[0].retired_value) * 8));
      fprintf(file, "\t\tstride           :%4lu bits  // RISCV64 integer size. Competition opportunity: truncate stride to far fewer bits based on stride distribution of stride-predictable instructions.\n", (sizeof(SVP[0].stride) * 8));
      fprintf(file, "\t\tinstance ctr     :%4lu bits  // formula: (uint64_t)ceil(log2((double)VPQsize))\n", (uint64_t)ceil(log2( (double)vpq_size) ) );
      fprintf(file, "\t\t-------------------------\n");
      fprintf(file, "\t\tbits/SVP entry   :%4lu bits\n", SVP_bits);

      uint64_t SVP_entries = pow(2, index_len);
      uint64_t total_SVP = SVP_bits * SVP_entries;
      
      fprintf(file, "\tTotal storage cost (bits) = (%lu SVP entries x %lu bits/SVP entry) = %lu bits\n", SVP_entries, SVP_bits, total_SVP);
      fprintf(file, "\tTotal storage cost (bytes) = %6.2f B (%3.2f KB)\n",((float)total_SVP/8), ((float)total_SVP/(8*1024)) );
   }
}
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
void Value_Prediction::VP_stats(FILE* stats_log){

   float total_vp = vpmeas_ineligible+vpmeas_eligible;
   float pct_ineligible = ((float)vpmeas_ineligible / total_vp)*(100);
   float pct_eligible = ((float)vpmeas_eligible/total_vp)*(100);
   float pct_miss = ((float)vpmeas_miss/total_vp)*(100);
   float pct_conf_corr = ((float) vpmeas_conf_corr/total_vp)*(100);
   float pct_conf_incorr = ((float)vpmeas_conf_incorr/total_vp)*(100);
   float pct_unconf_corr = ((float)vpmeas_unconf_corr/total_vp)*(100);
   float pct_unconf_incorr = ((float)vpmeas_unconf_incorr/total_vp)*(100);

   fprintf(stats_log, "VPU MEASUREMENTS-----------------------------------\n");
   fprintf(stats_log, "vpmeas_ineligible         : %10d (%6.2f%%) // Not eligible for value prediction.\n", vpmeas_ineligible,pct_ineligible);
   fprintf(stats_log, "vpmeas_eligible           : %10d (%6.2f%%) // Eligible for value prediction.\n", vpmeas_eligible,pct_eligible );
   fprintf(stats_log, "   vpmeas_miss            : %10d (%6.2f%%) // VPU was unable to generate a value prediction (e.g., SVP miss).\n", vpmeas_miss,pct_miss);
   fprintf(stats_log, "   vpmeas_conf_corr       : %10d (%6.2f%%) // VPU generated a confident and correct value prediction.\n", vpmeas_conf_corr,pct_conf_corr);
   fprintf(stats_log, "   vpmeas_conf_incorr     : %10d (%6.2f%%) // VPU generated a confident and incorrect value prediction. (MISPREDICTION)\n", vpmeas_conf_incorr,pct_conf_incorr);
   fprintf(stats_log, "   vpmeas_unconf_corr     : %10d (%6.2f%%) // VPU generated an unconfident and correct value prediction. (LOST OPPORTUNITY)\n", vpmeas_unconf_corr,pct_unconf_corr );
   fprintf(stats_log, "   vpmeas_unconf_incorr   : %10d (%6.2f%%) // VPU generated an unconfident and incorrect value prediction.\n", vpmeas_unconf_incorr,pct_unconf_incorr );

}
//////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_PRINTS
//////////////////////////////////////////////////////////////////////////////////
void Value_Prediction::svp_debug_prints(FILE* stats_log){
   fprintf(stats_log, "SVP entry #:   tag(hex)   conf   retired_value   stride   instance\n");

   for (int i = 0; i < svp_entries; ++i) {
      fprintf(stats_log, "%11d:  %8x  %5lu  %15lu  %9d  %9lu\n",
              i,
              SVP[i].tag,
              SVP[i].conf,
              SVP[i].retired_value,
              SVP[i].stride,
              SVP[i].instance);
   }

}

void Value_Prediction::vpq_debug_prints(FILE* stats_log){
   fprintf(stats_log, "VPQ entry #:   PC(hex)   PCtag(hex)   PCindex(hex)\n");

   for (int i = vpq_head; i != vpq_tail ; i=(i+1)%vpq_size ) {
      fprintf(stats_log, "%11d:  %8x     %10x      %10x\n",
               i,
               VPQ[i].PC,
               VPQ[i].PC_tag,
               VPQ[i].PC_index,
               VPQ[i].value
               );
   }
}

#endif