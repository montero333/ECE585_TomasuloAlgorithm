/*
######################################################################################################################
  Author:   Arthur J. Miller
  FileName: main.cpp
  Full project can be found at github.com/milleraj66/ECE585_TomasuloAlgorithm
  Date:
            Start:  03-01-2016
            Update: 03-08-2016
            Update: 03-09-2016
            Update: 03-09-2016
  Purpose:  ECE 585 Tomasulo Algorithm
            Driver file for Tomasulo algorithm
######################################################################################################################
This program follows the algorithm as stated by the following sources;
Computer Architecture : A Quantitative Approach (5th Edition) by Hennessy, John L., Patterson, David A.
https://en.wikipedia.org/wiki/Tomasulo_algorithm#Implementation_concepts
Dr. Chandra CPP

ALGORITHM:
	ISSUE:
		// DELAY: 1 Clock
		// determine reservation station destination for given instruction
	    // r is the given reservation station
	    Switch(INST.op):
		case(add):  r=add; 	goto ISSUE
		case(mult): r= mult; 	goto ISSUE
		case(div):  r = div;	goto ISSUE
		// check if spot in given reservation station is available
		if(RS[r].busy = false):
			// if operand rs is available -> set value of operand (Vj) to given register value
			// else point operand to the reservation station (Qj) that will give the operand value
			if(RegStatus[rs].Qi == 0):
				RS[r].Vj = Reg[rs]; RS[r].Qj = 0;
			else
				RS[r].Qj = RegStatus[r].Qi;
			// if operand rt is available -> set value of operand (Vk) to given register value
			// else point operand to the reservation station (Qk) that will give the operand value
			if(RegStatus[rt].Qi == 0):
				RS[r].Vj = Reg[rt]; RS[r].Qj = 0;
			else
				RS[r].Qj = RegStatus[r].Qi;
		// given reservation station is now busy until write back stage is completed.
		RS[r].busy = true; RegStatus[rd].Qi = true
	EXECUTE:
 		// check each reservation station to see if both operands are ready
		// Delay: Switch(INST.op)
		//		case(add): 	clock += 4;
		//		case(mult): 	clock += 12;
		//		case(div):	clock += 38;
		// wait for both operands to be available and execute given instructions operation
		if(RS[r].Qj == 0 && RS[r].Qk == 0):
			switch(INST.op):
				case(add):	temp = Vj +- Vk
				case(mult):	temp = Vj * Vk
				case(div):	temp = Vj / Vk
	WRITEBACK:
 		// Check each reservation station to see if operaional delay is done -> result is ready
		// Delay: 1 clock
		// for all x registers
		// if RegStatus points to the given reservation station r set that register[x] equal to executed result	
		if(RegStatus[x].Qi==r):
			Reg[x] = temp; RegStatus[x].Qi = false;
		// check if any reservation stations are waiting for the given result as an operand	
		if(RS[x].Qj==r):
			RS[x].Vi=temp; RS[x].Qj=false;
		if(RS[x].Qk==r):
			RS[x].Vk=temp; RS[x].Qk=false;
		// The given reservation station is can now be used again		
		RS[r].busy = false;
 */

#include <iostream>
#include <iomanip>          // Print Table Formatting
#include <vector>
#include "RS.h"             // Reservation Station Class
#include "Instruction.h"    // MIPS Style Instruction Class
#include "RegStatus.h"      // Register Status Class (Shows which RS the given register is waiting for)

using namespace std;

//####################################################################################################################
//*** Define Architecture
// RESERVATION STATION NUMBER
const int Num_Total_RS = 9;
const int Num_ADD_RS = 4;
const int Num_MULT_RS = 2;
const int Num_DIV_RS = 3;
// RESERVATION STATION LATENCY
const int ADD_Lat = 4;
const int MULT_Lat = 12;
const int DIV_Lat = 38;
// Global Clock
int Clock = 0;
// Opcode Values
const int AddOp = 0;
const int SubOp = 1;
const int MultOp = 2;
const int DivOp = 3;
// Counter for current instruction to issue
int currentInst_ISSUE = 0;
// Temporary fix for errors due to RS names being numbers -> errors with REG/RS/REGSTATUS == zero
const int RegStatusEmpty = 1000;
const int OperandAvailable = 1001;
const int OperandInit = 1002;
//####################################################################################################################

//####################################################################################################################
// Driver Functions
int ISSUE(vector<Instruction>& Inst,vector<RS>& ResStat,vector<RegStatus>& RegStat,vector<int>& Register);
void EXECUTE(vector<Instruction>& Inst,vector<RS>& ResStat,vector<RegStatus>& RegStat,vector<int>& Register);
void WRITEBACK(vector<Instruction>& Inst,vector<RS>& ResStat,vector<RegStatus>& RegStat,vector<int>& Register);
// Helper functions
void printRegisterStatus(vector<RegStatus> RegisterStatusVector);
void printReservationStations(vector<RS> ReservationStationsVector);
void printRegisters(vector<int> RegistersVector);
void printInstructions(vector<Instruction> InstructionsVector);
void printTimingTable(vector<Instruction> INST);
//####################################################################################################################

//####################################################################################################################
// MAIN DRIVER
int main(){
    //**** START Define Architecture
    // Input program instructions
    Instruction I0(1,2,3,AddOp),I1(4,1,5,AddOp),I2(6,7,8,SubOp),I3(9,4,10,MultOp),I4(11,12,6,DivOp);
    vector<Instruction> Inst = {I0,I1,I2,I3,I4};
    // Define # of each type of reservation station
    RS ADD1(AddOp,OperandInit),ADD2(AddOp,OperandInit),ADD3(AddOp,OperandInit),ADD4(AddOp,OperandInit);
    RS MULT1(MultOp,OperandInit),MULT2(MultOp,OperandInit);
    RS DIV1(DivOp,OperandInit),DIV2(DivOp,OperandInit),DIV3(DivOp,OperandInit);
    vector<RS> ResStation = {ADD1,ADD2,ADD3,ADD4,MULT1,MULT2,DIV1,DIV2,DIV3};
    // Initialize register status vector
    RegStatus F0(RegStatusEmpty),F1(RegStatusEmpty),F2(RegStatusEmpty),F3(RegStatusEmpty),F4(RegStatusEmpty),
            F5(RegStatusEmpty),F6(RegStatusEmpty),F7(RegStatusEmpty),F8(RegStatusEmpty),F9(RegStatusEmpty),
            F10(RegStatusEmpty),F11(RegStatusEmpty),F12(RegStatusEmpty);
    vector<RegStatus> RegisterStatus = {F0,F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12};
    // Initialze register vector
    vector<int> Register = {5000,1,2,3,4,5,6,7,8,9,10,11,12};
    //**** END Define Architecture

    cout << "INITIAL VALUES:" << endl;
    printInstructions(Inst);
    printRegisters(Register);
    printRegisterStatus(RegisterStatus);
    printReservationStations(ResStation);
    cout << endl;

    //**** START functional loop
    while(Clock < 50){
        // Datapath
        Clock++;
        cout << "System Clock: " << endl;
        cout << "Clock: " << Clock << endl;

        ISSUE(Inst,ResStation,RegisterStatus,Register);

		EXECUTE(Inst,ResStation,RegisterStatus,Register);

		WRITEBACK(Inst,ResStation,RegisterStatus,Register);

        //printTimingTable(Inst);
	}//**** End functional loop
    printRegisters(Register);
}//**** END MAIN DRIVER
//####################################################################################################################

//####################################################################################################################
// Datapath FUNCTIONS
int ISSUE(vector<Instruction>& INST,vector<RS>& RESSTATION,vector<RegStatus>& REGSTATUS,vector<int>& REG){
    //Fetch
    //**** check if spot in given reservation station is available
    int r = 0;
    // r is the current instruction to be issued's operation code(add,sub,mult,div)
    // If all instructions have been issued then stop issueing for rest of program
    if(currentInst_ISSUE > INST.size())
            return 0;
    r = INST[currentInst_ISSUE].op;
    // determine if there is an open RS of r type. if yes -> r = that open spot.
    switch(r){
        case AddOp:
            for(int i=Num_ADD_RS-Num_ADD_RS;i<Num_ADD_RS;i++){
                if(!RESSTATION[i].busy){
                    r = i; // RS are definced 1 -> 10 rather than from 0
                    currentInst_ISSUE++;
                    RESSTATION[i].op = AddOp;
                    break;
                }
            }
            break;
        case SubOp:
            for(int i=Num_ADD_RS-Num_ADD_RS;i<Num_ADD_RS;i++){
                if(!RESSTATION[i].busy){
                    r = i; // RS are definced 1 -> 10 rather than from 0
                    currentInst_ISSUE++;
                    RESSTATION[i].op = SubOp;
                    break;
                }
            }
            break;
        case MultOp:
            for(int i=Num_ADD_RS;i<Num_ADD_RS+Num_MULT_RS;i++){
                if(!RESSTATION[i].busy){
                    r = i; // RS are definced 1 -> 10 rather than from 0
                    currentInst_ISSUE++;
                    RESSTATION[i].op = MultOp;
                    break;
                }
            }
            break;
        case DivOp:
            for(int i=Num_ADD_RS+Num_MULT_RS;i<Num_ADD_RS+Num_MULT_RS+Num_DIV_RS;i++){
                if(!RESSTATION[i].busy){
                    r = i;// RS are definced 1 -> 10 rather than from 0
                    currentInst_ISSUE++;
                    RESSTATION[i].op = DivOp;
                    break;
                }
            }
            break;
        default:
            break;
    }
    //**** Initialize characteristics of issued instruction
    // if operand rs is available -> set value of operand (Vj) to given register value
    // else point operand to the reservation station (Qj) that will give the operand value
    // NOTE: since currentInst was in incremented we must do currentINST_ISSUE-1
    if(REGSTATUS[INST[currentInst_ISSUE-1].rs].Qi == RegStatusEmpty){
        RESSTATION[r].Vj = REG[INST[currentInst_ISSUE-1].rs];
        RESSTATION[r].Qj = OperandAvailable;
    }
    else{
        RESSTATION[r].Qj = REGSTATUS[INST[currentInst_ISSUE-1].rs].Qi;
    }
    // if operand rt is available -> set value of operand (Vk) to given register value
    // else point operand to the reservation station (Qk) that will give the operand value
    if(REGSTATUS[INST[currentInst_ISSUE-1].rt].Qi == RegStatusEmpty){
        RESSTATION[r].Vk = REG[INST[currentInst_ISSUE-1].rt];
        RESSTATION[r].Qk = OperandAvailable;
    }
    else{
        RESSTATION[r].Qk = REGSTATUS[INST[currentInst_ISSUE-1].rt].Qi;
    }
    // given reservation station is now busy until write back stage is completed.
    RESSTATION[r].busy = true;
    // set reservation station instuction number == current instruction
    RESSTATION[r].instNum = currentInst_ISSUE-1;
    // set clock cycle for issue time
    INST[r].issueClock = Clock;
    // The register status Qi is set to the current instructions reservation station location r
    REGSTATUS[INST[currentInst_ISSUE-1].rd].Qi = r;

    return 1;
}//END ISSUE()
void EXECUTE(vector<Instruction>& INST,vector<RS>& RESSTATION,vector<RegStatus>& REGSTATUS,vector<int>& REG){
    // check each reservation station to see if both operands are ready
    // The current reservation station is r
    for (int r=0;r<Num_Total_RS;r++){
        // if both operands are available then execute given instructions operation
        // and set resultReady flag to true so that result can be written back to CDB
        if(RESSTATION[r].Qj == OperandAvailable && RESSTATION[r].Qk == OperandAvailable){
            // Set clock cycle when execution begins
            INST[RESSTATION[r].instNum].executeClockBegin = Clock;
            // when execution starts we must wait the given latency number of clock cycles before making result
            // available to WriteBack
            // Delay: Switch(INST.op)
            //		case(add): 	clock += 4;
            //		case(mult): 	clock += 12;
            //		case(div):	clock += 38;
            RESSTATION[r].lat++;
            switch(RESSTATION[r].op){
                case(AddOp):
                    if(RESSTATION[r].lat == ADD_Lat){
                        RESSTATION[r].result = RESSTATION[r].Vj + RESSTATION[r].Vk;
                        cout << "ADD result: " << RESSTATION[r].result << endl;
                        RESSTATION[r].resultReady = true; // Result is ready to be writenback
                        RESSTATION[r].lat = 0;
                        // Set clock cycle when execution ends
                        INST[RESSTATION[r].instNum].executeClockEnd = Clock;
                    }
                case(SubOp):
                    if(RESSTATION[r].lat == ADD_Lat){
                        RESSTATION[r].result = RESSTATION[r].Vj - RESSTATION[r].Vk;
                        cout << "SUB result: " << RESSTATION[r].result << endl;
                        RESSTATION[r].resultReady = true; // Result is ready to be writenback
                        RESSTATION[r].lat = 0;
                        // Set clock cycle when execution ends
                        INST[RESSTATION[r].instNum].executeClockEnd = Clock;
                    }
                case(MultOp):
                    if(RESSTATION[r].lat == MULT_Lat){
                        cout << "RS (r):  " << r << endl;
                        RESSTATION[r].result = RESSTATION[r].Vj * RESSTATION[r].Vk;
                        cout << "MULT result: " << RESSTATION[r].result << endl;
                        RESSTATION[r].resultReady = true; // Result is ready to be writenback
                        cout << "MULT resultReady: " << RESSTATION[r].resultReady << endl;
                        RESSTATION[r].lat = 0;
                        // Set clock cycle when execution ends
                        INST[RESSTATION[r].instNum].executeClockEnd = Clock;
                    }
                case(DivOp):
                    if(RESSTATION[r].lat == DIV_Lat){
                        RESSTATION[r].result = RESSTATION[r].Vj / RESSTATION[r].Vk;
                        cout << "DIV result: " << RESSTATION[r].result << endl;
                        RESSTATION[r].resultReady = true; // Result is ready to be writenback
                        RESSTATION[r].lat = 0;
                        // Set clock cycle when execution ends
                        INST[RESSTATION[r].instNum].executeClockEnd = Clock;
                    }
                default:
                    break;
            }
        }
    }

}//END EXECUTE()
void WRITEBACK(vector<Instruction>& INST,vector<RS>& RESSTATION,vector<RegStatus>& REGSTATUS,vector<int>& REG){
    // Check each reservation station to see if operational delay is done -> result is ready
    int WBclock = 0; // if we WRITEBACK then we need a clock delay of 1
    for(int r=0;r<Num_Total_RS;r++){
        // if result ready write back to CDB -> Register,and reservation stations
        if(RESSTATION[r].resultReady){
            // set clock cycle when write back occured. (Must add one because increment happens after loop)
            INST[RESSTATION[r].instNum].writebackClock = Clock+1;
            cout << "resultReady True (r):  " << r << endl;
            // for all x registers
            WBclock++; // There is a result being written back
            for(int x=0;x<REG.size();x++){
                // if RegStatus points to the given reservation station r set that register[x] equal to executed result
                if(REGSTATUS[x].Qi==r){
                    cout << "RegStatus points (Qi) to (r): " << x << endl;
                    // Write back to Registers
                    cout << "REG[x] before: " << REG[x] << endl;
                    REG[x] = RESSTATION[r].result;
                    cout << "REG[x] after: " << REG[x] << endl;
                    REGSTATUS[x].Qi = RegStatusEmpty;
                }
                // check if any reservation stations are waiting for the given result as an operand
                // Write back to reservation stations
                // Given RS is not longer waiting for this operand value
                if(RESSTATION[x].Qj==r){
                    RESSTATION[x].Vj=RESSTATION[r].result;
                    RESSTATION[x].Qj=OperandAvailable;
                }
                if(RESSTATION[x].Qk==r){
                    RESSTATION[x].Vk=RESSTATION[r].result;
                    RESSTATION[x].Qk=OperandAvailable;
                }
            }
            // The given reservation station can now be used again
            RESSTATION[r].resultReady = false; // Set RS resultReady back to false
            RESSTATION[r].busy = false;
            RESSTATION[r].Qj = OperandInit;
            RESSTATION[r].Qk = OperandInit;
            RESSTATION[r].Vj = 0;
            RESSTATION[r].Vk = 0;
        }
    }
    // 1 cycle delay for WB
    if(WBclock>0)
        Clock++;
}//END WRITEBACK()
//####################################################################################################################

//####################################################################################################################
// Helper Functions
void printRegisterStatus(vector<RegStatus> RegisterStatusVector){
    cout << "Register Status: " << endl;
    for(int i=0; i<RegisterStatusVector.size(); i++)
        cout << RegisterStatusVector[i].Qi << ' ';
    cout << endl;
}
void printReservationStations(vector<RS> ReservationStationsVector){
    for(int i=0; i<ReservationStationsVector.size(); i++)
        cout << "RS #: " << i << "  Busy: " << ReservationStationsVector[i].busy << "  op: " <<
                ReservationStationsVector[i].op << "  Vj: " << ReservationStationsVector[i].Vj << "  Vk: " <<
                ReservationStationsVector[i].Vk << "  Qj: " << ReservationStationsVector[i].Qj << "  Qk: " <<
                ReservationStationsVector[i].Qk << endl;
}
void printRegisters(vector<int> RegistersVector){
    cout << "Register Content:" << endl;
    for(int i=0; i<RegistersVector.size(); i++)
        cout << RegistersVector[i] << ' ';
    cout << endl;
}
void printInstructions(vector<Instruction> InstructionsVector){
    for(int i=0; i<InstructionsVector.size(); i++)
        cout << "Instruction #: " << i << "  Operation: " << InstructionsVector[i].op << "  " <<
                InstructionsVector[i].rd << " <- " << InstructionsVector[i].rs << " op " <<
                InstructionsVector[i].rt << endl;
}
void printTimingTable(vector<Instruction> INST){
    char separator    = ' ';
    const int width     = 4;
    char lineSeperator = '-';
    const int lineWidth = 30;

    // Define column labels
    cout << left << setw(width) << setfill(separator) << "Inst";
    cout << left << setw(width) << setfill(separator) << "Issue";
    cout << left << setw(width) << setfill(separator) << "Execute";
    cout << left << setw(width) << setfill(separator) << "WB";
    cout << endl;
    cout << left << setw(lineWidth) << setfill(lineSeperator);
    cout << endl;
    // Define Row Labels and values
    for(int i=0;i<INST.size();i++){
        cout << left << setw(width) << setfill(separator) << "I" << i;
        cout << left << setw(width) << setfill(separator) << INST[i].issueClock;
        cout << left << setw(width) << setfill(separator) << INST[i].executeClockBegin <<
                "-" << INST[i].executeClockEnd;
        cout << left << setw(width) << setfill(separator) << INST[i].writebackClock;
        cout << endl;
    }

}
//####################################################################################################################
