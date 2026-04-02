#include "dfa.hpp"

// State-lihat penjelasan pada dfa.hpp

State::State() {
    assignCharID("null");
    stateIdx = -1;
    finState = false;
}

State::State(const char* charID, int16_t newStateIdx, bool isFinState) {
    assignCharID(charID);
    stateIdx = newStateIdx;
    finState = isFinState;
}

State::State(const State& other) {
    finState = other.finState;
    stateIdx = other.stateIdx;
    assignCharID(other.getStateCharID());
}

State::State(State&& other) noexcept {
    finState = other.finState;
    stateIdx = other.stateIdx;

    assignCharID(other.stateCharID);
}

bool State::isFinalState() const {
    return finState;
}

int16_t State::getStateIdx() const {
    return stateIdx;
}

const char* State::getStateCharID() const {
    return stateCharID;
}

bool State::compCharID(const char* otherCharID) const {
    if (strcmp(stateCharID, otherCharID) == 0) {
        return true;
    }
    return false;
}

void State::assignCharID(const char* newCharID) {
    strncpy(stateCharID, newCharID, sizeof(stateCharID) - 1);
    stateCharID[N_STATE_CHAR_ID - 1] = '\0';
}

bool State::isNullState() const {
    if (stateIdx == -1) return true;
    return false;
}

State::~State(){}

// DFA-Lihat penjelasan pada dfa.hpp

DFA::DFA(): currStateIdx(-1) {
}

void DFA::loadConfig(std::string path) {
    std::fstream fs;
    fs.open(path, std::ios::in);
    if (!fs) {
        std::cout << "Gagal membuka file, file config tidak berhasil di load." << "\n";
    } else {
        std::string line, kw, body, tk1, tk2, tk3;
        while (std::getline(fs, line)) {
            std::istringstream iss(line);
            iss >> kw >> tk1 >> tk2 >> tk3;
            
            if (kw == "START") {
                removeTrailingWhitespace(body);
                addStateWithIdx(0, State(body.c_str(), 0, false));
            } else if (kw == "FINAL") {
                
            }
        }
    }
    
}

void DFA::exportDFAConfig(std::string path) const {
    ;
}

void DFA::visualizeProcess(std::string path) const {
    ;
}

const State& DFA::getState() const {
    if (currStateIdx < 0|| currStateIdx > states.size()) {
        std::cout << "DFA berada dalam current state tidak valid. \n";
        return State(); //mengembalikan null state
    }
    return states[currStateIdx];
}

void DFA::next(unsigned char c) {
   int16_t newStateIdx = transTable[currStateIdx][c];
   // menghasilkan -1 jika state tidak ditemukan, artinya pada saat demikian state menjadi tidak valid atau null;
   currStateIdx = newStateIdx; 
}

void DFA::resetState() {
    currStateIdx = 0; //konvensi bahwa stateIdx 0 adalah start state;
}

int16_t DFA::addUniqueState(const char* newCharID, bool newFinState){
    bool found = false;
    int16_t addedStateIdx = states.size();
    for (int16_t i = 0; i < states.size(); i++){
        if (states[i].compCharID(newCharID)){
            found = true;
            addedStateIdx = i;
        }
    }
    if (!found){
        states.push_back(State(newCharID, addedStateIdx, newFinState));
    }
    return addedStateIdx;
}

void DFA::addStateWithIdx(int16_t idx, State nState) {
    states.insert(states.begin() + idx, nState);
}

void DFA::setCurrentState(int16_t newStateIdx) {
    currStateIdx = newStateIdx;
}

void DFA::removeTrailingWhitespace(std::string& str) const{
    const std::string WHITESPACE = " \t\n\r\f\v";
    size_t end = str.find_last_not_of(WHITESPACE);
    if (end != std::string::npos) {
        str.erase(end + 1);
    } else {
        str.clear();
    }
}

DFA::~DFA(){}