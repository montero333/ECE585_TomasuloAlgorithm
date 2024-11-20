#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
using namespace std;

// Constants (These can be made configurable)
const int DEFAULT_ADD_LAT = 4;    // Latency for ADD/SUB operations
const int DEFAULT_MULT_LAT = 12; // Latency for MULT operation
const int DEFAULT_DIV_LAT = 38;  // Latency for DIV operation
const int ISSUE_LAT = 1;         // Latency for issuing an instruction
const int WRITEBACK_LAT = 1;     // Latency for writing back a result

// Define operation codes
enum Operation { ADD, SUB, MULT, DIV, NONE };

// Instruction Class: Represents an instruction with its operands, operation type, and timestamps.
class Instruction {
public:
    int rd, rs, rt;        // Destination and source registers
    Operation op;          // Operation type (ADD, SUB, MULT, DIV)
    int issueClock;        // Clock cycle when issued
    int executeClockBegin; // Clock cycle when execution starts
    int executeClockEnd;   // Clock cycle when execution ends
    int writebackClock;    // Clock cycle when result is written back

    Instruction(int RD, int RS, int RT, Operation OP)
        : rd(RD), rs(RS), rt(RT), op(OP), issueClock(0),
          executeClockBegin(0), executeClockEnd(0), writebackClock(0) {}
};

// Register Status Class: Tracks if a register is busy and which reservation station is writing to it.
class RegisterStatus {
public:
    bool busy; // Is the register busy?
    int Qi;    // Index of the reservation station producing the result

    RegisterStatus() : busy(false), Qi(-1) {}
};

// Reservation Station Class: Represents a functional unit for executing operations.
class ReservationStation {
public:
    bool busy;         // Is the station busy?
    int Qj, Qk;        // Reservation stations producing operands
    int Vj, Vk;        // Operands
    int latency;       // Execution latency
    Operation op;      // Operation type
    int result;        // Result of the operation
    bool resultReady;  // Is the result ready?
    int instNum;       // Associated instruction number
    int issueLat;      // Remaining issue latency
    int writebackLat;  // Remaining writeback latency

    ReservationStation() : busy(false), Qj(-1), Qk(-1), Vj(0), Vk(0),
                           latency(0), op(NONE), result(0), resultReady(false),
                           instNum(-1), issueLat(0), writebackLat(0) {}
};

// Global Clock
int Clock = 0; // Current clock cycle
bool Done = false; // Simulation completion flag
int TotalWritebacks = 0; // Count of completed instructions
int CurrentIssueIndex = 0; // Index of the instruction to be issued next

// Function Prototypes
void issueInstruction(vector<Instruction>&, vector<ReservationStation>&, vector<RegisterStatus>&, vector<int>&);
void executeInstructions(vector<Instruction>&, vector<ReservationStation>&);
void writebackResults(vector<Instruction>&, vector<ReservationStation>&, vector<RegisterStatus>&, vector<int>&);
void printStatus(const vector<Instruction>&, const vector<ReservationStation>&, const vector<int>&, const vector<RegisterStatus>&);

int main() {
    // Initialize Instructions (Example set)
    vector<Instruction> instructions = {
        Instruction(1, 2, 3, ADD),
        Instruction(4, 1, 5, ADD),
        Instruction(6, 7, 8, SUB),
        Instruction(9, 4, 10, MULT),
        Instruction(11, 12, 6, DIV),
        Instruction(8, 1, 5, MULT),
        Instruction(7, 2, 3, MULT)
    };

    // Initialize Reservation Stations
    vector<ReservationStation> reservationStations(9); // Example size
    vector<RegisterStatus> registerStatus(13); // Example size
    vector<int> registers(13, 0); // Initialize register contents

    // Main simulation loop
    while (!Done) {
        Clock++; // Increment clock

        // Process the three stages
        issueInstruction(instructions, reservationStations, registerStatus, registers);
        executeInstructions(instructions, reservationStations);
        writebackResults(instructions, reservationStations, registerStatus, registers);

        // Print the status after each clock cycle
        printStatus(instructions, reservationStations, registers, registerStatus);

        // Check if all instructions have completed
        if (TotalWritebacks == instructions.size()) Done = true;
    }
    return 0;
}

// Issue Stage Function: Issues an instruction if a free reservation station is available
void issueInstruction(vector<Instruction>& instructions, vector<ReservationStation>& reservationStations, vector<RegisterStatus>& registerStatus, vector<int>& registers) {
    if (CurrentIssueIndex >= instructions.size()) return; // All instructions issued

    Instruction& inst = instructions[CurrentIssueIndex]; // Current instruction
    for (auto& rs : reservationStations) {
        if (!rs.busy) { // Found an available reservation station
            rs.busy = true;
            rs.op = inst.op;
            rs.instNum = CurrentIssueIndex;
            rs.issueLat = ISSUE_LAT;

            // Check if source operands are available
            rs.Vj = (!registerStatus[inst.rs].busy) ? registers[inst.rs] : (rs.Qj = registerStatus[inst.rs].Qi, 0);
            rs.Vk = (!registerStatus[inst.rt].busy) ? registers[inst.rt] : (rs.Qk = registerStatus[inst.rt].Qi, 0);

            // Mark destination register as busy
            registerStatus[inst.rd].busy = true;
            registerStatus[inst.rd].Qi = &rs - &reservationStations[0];

            // Record issue time
            inst.issueClock = Clock;
            CurrentIssueIndex++;
            break;
        }
    }
}

// Execute Stage Function: Executes instructions if operands are ready
void executeInstructions(vector<Instruction>& instructions, vector<ReservationStation>& reservationStations) {
    for (auto& rs : reservationStations) {
        if (rs.busy && rs.Qj == -1 && rs.Qk == -1) { // Operands available
            if (rs.latency == 0) instructions[rs.instNum].executeClockBegin = Clock; // Record execution start
            rs.latency++;

            // Check if execution is complete
            int operationLatency = (rs.op == ADD || rs.op == SUB) ? DEFAULT_ADD_LAT : (rs.op == MULT) ? DEFAULT_MULT_LAT : DEFAULT_DIV_LAT;
            if (rs.latency >= operationLatency) {
                rs.result = (rs.op == ADD) ? rs.Vj + rs.Vk : (rs.op == SUB) ? rs.Vj - rs.Vk : (rs.op == MULT) ? rs.Vj * rs.Vk : rs.Vj / rs.Vk;
                rs.resultReady = true;
                rs.latency = 0;
                instructions[rs.instNum].executeClockEnd = Clock; // Record execution end
            }
        }
    }
}

// Writeback Stage Function: Writes results back to registers and clears reservation stations
void writebackResults(vector<Instruction>& instructions, vector<ReservationStation>& reservationStations, vector<RegisterStatus>& registerStatus, vector<int>& registers) {
    for (auto& rs : reservationStations) {
        if (rs.resultReady) {
            if (rs.writebackLat < WRITEBACK_LAT) { rs.writebackLat++; continue; }

            Instruction& inst = instructions[rs.instNum];
            if (inst.writebackClock == 0) inst.writebackClock = Clock;

            // Write result to destination register
            registers[inst.rd] = rs.result;
            registerStatus[inst.rd].busy = false;
            registerStatus[inst.rd].Qi = -1;

            // Update dependent reservation stations
            for (auto& otherRs : reservationStations) {
                if (otherRs.Qj == &rs - &reservationStations[0]) otherRs.Vj = rs.result, otherRs.Qj = -1;
                if (otherRs.Qk == &rs - &reservationStations[0]) otherRs.Vk = rs.result, otherRs.Qk = -1;
            }

            // Clear reservation station
            rs.busy = false;
            rs.resultReady = false;
            rs.writebackLat = 0;
            TotalWritebacks++;
        }
    }
}

// Print Status Function: Displays the status of instructions, registers, and reservation stations
void printStatus(const vector<Instruction>& instructions, const vector<ReservationStation>& reservationStations, const vector<int>& registers, const vector<RegisterStatus>& registerStatus) {
    cout << "Clock Cycle: " << Clock << endl;

    // Print instruction timestamps
    cout << "Instructions: ";
    for (const auto& inst : instructions) {
        cout << "[" << inst.issueClock << ", " << inst.executeClockBegin << "-" << inst.executeClockEnd << ", " << inst.writebackClock << "] ";
    }
    cout << endl;

    // Print register contents
    cout << "Registers: ";
    for (const auto& reg : registers) cout << reg << " ";
    cout << endl;
}
