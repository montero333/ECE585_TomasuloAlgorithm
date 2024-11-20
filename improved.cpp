#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
using namespace std;

// Constants (These can be made configurable)
const int DEFAULT_ADD_LAT = 4;
const int DEFAULT_MULT_LAT = 12;
const int DEFAULT_DIV_LAT = 38;
const int ISSUE_LAT = 1;
const int WRITEBACK_LAT = 1;

// Define operation codes
enum Operation { ADD, SUB, MULT, DIV, NONE };

// Instruction Class
class Instruction {
public:
    int rd, rs, rt;
    Operation op;
    int issueClock, executeClockBegin, executeClockEnd, writebackClock;

    Instruction(int RD, int RS, int RT, Operation OP)
        : rd(RD), rs(RS), rt(RT), op(OP), issueClock(0),
          executeClockBegin(0), executeClockEnd(0), writebackClock(0) {}
};

// Register Status Class
class RegisterStatus {
public:
    bool busy;
    int Qi; // Reservation Station producing the result

    RegisterStatus() : busy(false), Qi(-1) {}
};

// Reservation Station Class
class ReservationStation {
public:
    bool busy;
    int Qj, Qk;
    int Vj, Vk;
    int latency;
    Operation op;
    int result;
    bool resultReady;
    int instNum;
    int issueLat, writebackLat;

    ReservationStation()
        : busy(false), Qj(-1), Qk(-1), Vj(0), Vk(0),
          latency(0), op(NONE), result(0), resultReady(false),
          instNum(-1), issueLat(0), writebackLat(0) {}
};

// Global Clock
int Clock = 0;
bool Done = false;
int TotalWritebacks = 0;
int CurrentIssueIndex = 0;

// Function Prototypes
void issueInstruction(vector<Instruction>&, vector<ReservationStation>&,
                      vector<RegisterStatus>&, vector<int>&);
void executeInstructions(vector<Instruction>&, vector<ReservationStation>&);
void writebackResults(vector<Instruction>&, vector<ReservationStation>&,
                      vector<RegisterStatus>&, vector<int>&);
void printStatus(const vector<Instruction>&, const vector<ReservationStation>&,
                 const vector<int>&, const vector<RegisterStatus>&);

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
        Clock++;
        issueInstruction(instructions, reservationStations, registerStatus, registers);
        executeInstructions(instructions, reservationStations);
        writebackResults(instructions, reservationStations, registerStatus, registers);
        printStatus(instructions, reservationStations, registers, registerStatus);

        // Check if all instructions have completed
        if (TotalWritebacks == instructions.size()) Done = true;
    }

    return 0;
}

// Issue Stage Function
void issueInstruction(vector<Instruction>& instructions,
                      vector<ReservationStation>& reservationStations,
                      vector<RegisterStatus>& registerStatus, vector<int>& registers) {
    if (CurrentIssueIndex >= instructions.size()) return; // All instructions issued

    Instruction& inst = instructions[CurrentIssueIndex];
    for (auto& rs : reservationStations) {
        if (!rs.busy) { // Found an available reservation station
            rs.busy = true;
            rs.op = inst.op;
            rs.instNum = CurrentIssueIndex;
            rs.issueLat = ISSUE_LAT;

            // Check if operands are available
            if (!registerStatus[inst.rs].busy) {
                rs.Vj = registers[inst.rs];
                rs.Qj = -1; // Operand available
            } else {
                rs.Qj = registerStatus[inst.rs].Qi; // Waiting for operand
            }
            if (!registerStatus[inst.rt].busy) {
                rs.Vk = registers[inst.rt];
                rs.Qk = -1; // Operand available
            } else {
                rs.Qk = registerStatus[inst.rt].Qi; // Waiting for operand
            }

            // Mark the destination register as busy
            registerStatus[inst.rd].busy = true;
            registerStatus[inst.rd].Qi = &rs - &reservationStations[0];

            // Record the issue time
            inst.issueClock = Clock;
            CurrentIssueIndex++;
            break;
        }
    }
}

// Execute Stage Function
void executeInstructions(vector<Instruction>& instructions,
                         vector<ReservationStation>& reservationStations) {
    for (auto& rs : reservationStations) {
        if (rs.busy && rs.Qj == -1 && rs.Qk == -1) { // Operands available
            if (rs.latency == 0) { // First execution cycle
                instructions[rs.instNum].executeClockBegin = Clock;
            }
            rs.latency++;

            // Check if the operation has completed
            int operationLatency = (rs.op == ADD || rs.op == SUB) ? DEFAULT_ADD_LAT :
                                   (rs.op == MULT) ? DEFAULT_MULT_LAT :
                                   DEFAULT_DIV_LAT;

            if (rs.latency >= operationLatency) {
                rs.result = (rs.op == ADD) ? rs.Vj + rs.Vk :
                            (rs.op == SUB) ? rs.Vj - rs.Vk :
                            (rs.op == MULT) ? rs.Vj * rs.Vk : rs.Vj / rs.Vk;
                rs.resultReady = true;
                rs.latency = 0;
                instructions[rs.instNum].executeClockEnd = Clock;
            }
        }
    }
}

// Writeback Stage Function
void writebackResults(vector<Instruction>& instructions,
                      vector<ReservationStation>& reservationStations,
                      vector<RegisterStatus>& registerStatus, vector<int>& registers) {
    for (auto& rs : reservationStations) {
        if (rs.resultReady) {
            if (rs.writebackLat < WRITEBACK_LAT) {
                rs.writebackLat++;
                continue;
            }

            Instruction& inst = instructions[rs.instNum];
            if (inst.writebackClock == 0) {
                inst.writebackClock = Clock;
            }

            // Write result to the destination register
            registers[inst.rd] = rs.result;
            registerStatus[inst.rd].busy = false;
            registerStatus[inst.rd].Qi = -1;

            // Update any dependent reservation stations
            for (auto& otherRs : reservationStations) {
                if (otherRs.Qj == &rs - &reservationStations[0]) {
                    otherRs.Vj = rs.result;
                    otherRs.Qj = -1;
                }
                if (otherRs.Qk == &rs - &reservationStations[0]) {
                    otherRs.Vk = rs.result;
                    otherRs.Qk = -1;
                }
            }

            rs.busy = false;
            rs.resultReady = false;
            rs.writebackLat = 0;
            TotalWritebacks++;
        }
    }
}

// Print Status Function
void printStatus(const vector<Instruction>& instructions,
                 const vector<ReservationStation>& reservationStations,
                 const vector<int>& registers, const vector<RegisterStatus>& registerStatus) {
    cout << "Clock Cycle: " << Clock << endl;
    cout << "Instructions: ";
    for (const auto& inst : instructions) {
        cout << "[" << inst.issueClock << ", " << inst.executeClockBegin << "-"
             << inst.executeClockEnd << ", " << inst.writebackClock << "] ";
    }
    cout << endl;
    cout << "Registers: ";
    for (const auto& reg : registers) cout << reg << " ";
    cout << endl;
}
