#include "dfa.hpp"

// State-lihat penjelasan pada dfa.hpp

State::State()
{
    assignCharID("null");
    stateIdx = -1;
    finState = false;
}

State::State(const char *charID, int16_t newStateIdx, bool isFinState)
{
    assignCharID(charID);
    stateIdx = newStateIdx;
    finState = isFinState;
}

State::State(const State &other)
{
    finState = other.finState;
    stateIdx = other.stateIdx;
    assignCharID(other.getStateCharID());
}

State::State(State &&other) noexcept
{
    finState = other.finState;
    stateIdx = other.stateIdx;

    assignCharID(other.stateCharID);
}

bool State::isFinalState() const
{
    return finState;
}

int16_t State::getStateIdx() const
{
    return stateIdx;
}

const char *State::getStateCharID() const
{
    return stateCharID;
}

bool State::compCharID(const char *otherCharID) const
{
    if (strcmp(stateCharID, otherCharID) == 0)
    {
        return true;
    }
    return false;
}

void State::assignCharID(const char *newCharID)
{
    strncpy(stateCharID, newCharID, sizeof(stateCharID) - 1);
    stateCharID[N_STATE_CHAR_ID - 1] = '\0';
}

bool State::isNullState() const
{
    if (stateIdx == -1)
        return true;
    return false;
}

State::~State() {}

// DFA-Lihat penjelasan pada dfa.hpp

DFA::DFA() : currStateIdx(-1)
{
}

void DFA::loadConfig(std::string path)
{
    std::fstream fs;
    fs.open(path, std::ios::in);
    if (!fs)
    {
        std::cout << "Gagal membuka file, file config tidak berhasil di load." << "\n";
    }
    else
    {
        std::string line, kw = "", tk1 = "", tk2 = "", tk3 = "";
        bool ckKw, ckTk1, ckTk2, ckTk3;
        while (std::getline(fs, line))
        {
            std::istringstream stream(line);
            ckKw = static_cast<bool>(stream >> ckKw);
            ckTk1 = static_cast<bool>(stream >> tk1);
            ckTk2 = static_cast<bool>(stream >> tk2);
            ckTk3 = static_cast<bool>(stream >> tk3);

            if (ckKw)
            {
                if (kw == "START")
                {
                    if (ckTk1)
                    {
                        addStateWithIdx(0, State(tk1.c_str(), 0, false));
                    }
                }
                else if (kw == "FINAL")
                {
                    if (ckTk1 && ckTk2)
                    {
                        TokenType newTok = TokenType(tk1);
                        addTokenToTokenTypes(newTok.get_name(), newTok.get_type());
                        State newState = State(tk2.c_str(), addUniqueState(tk2.c_str(), true));

                        addToStateIDtoTokenID(newState.getStateIdx(), newTok.get_type())
                        
                    }
                }
                else
                {
                    // parser untuk format transisi
                    if (ckTk1, ckTk2)
                    {
                        int inputASCII = std::stoi(kw);
                        int16_t tk1Idx = findStateIdx(tk1.c_str());
                        if (tk1Idx < 0)
                        {
                            tk1Idx = addUniqueState(tk1.c_str(), false);
                        }
                        int16_t tk2Idx = findStateIdx(tk2.c_str());
                        if (tk2Idx < 0)
                        {
                            tk2Idx = addUniqueState(tk2.c_str(), false);
                        }
                        addTransition(tk1Idx, inputASCII, tk2Idx);
                    }
                }
            }
        }
    }
}

void DFA::exportDFAConfig(std::string path) const
{
    ;
}

void DFA::visualizeProcess(std::string path) const
{
    ;
}

const State &DFA::getState() const
{
    if (currStateIdx < 0 || currStateIdx > states.size())
    {
        std::cout << "DFA berada dalam current state tidak valid. \n";
        return State(); // mengembalikan null state
    }
    return states[currStateIdx];
}

void DFA::next(unsigned char c)
{
    int16_t newStateIdx = transTable[currStateIdx][c];
    // menghasilkan -1 jika state tidak ditemukan, artinya pada saat demikian state menjadi tidak valid atau null;
    currStateIdx = newStateIdx;
}

void DFA::resetState()
{
    currStateIdx = 0; // konvensi bahwa stateIdx 0 adalah start state;
}

int16_t DFA::addUniqueState(const char *newCharID, bool newFinState)
{
    int16_t idx;
    idx = findStateIdx(newCharID);
    if (idx < 0)
    {
        states.push_back(State(newCharID, states.size(), newFinState));
    }
    return idx;
}

int16_t DFA::findStateIdx(const char *newCharID)
{
    bool found = false;
    int16_t addedStateIdx = states.size();
    for (int16_t i = 0; i < states.size(); i++)
    {
        if (states[i].compCharID(newCharID))
        {
            found = true;
            addedStateIdx = i;
        }
    }
    if (!found)
    {
        return -1;
    }
    return addedStateIdx;
}

void DFA::addStateWithIdx(int16_t idx, State nState)
{
    states.insert(states.begin() + idx, nState);
}

void DFA::setCurrentState(int16_t newStateIdx)
{
    currStateIdx = newStateIdx;
}

void DFA::addTransition(int16_t state1, int input, int16_t state2)
{
    transTable[state1][input] = state2;
}

void DFA::addTokenToTokenTypes(Token newTok)
{
    tokensType.push_back(newTok);
}

void DFA::addTokenTypeIDMapping(std::string name, int tokId){
    tokensType.insert(std::make_pair(name, tokId));
}

void DFA::addToStateIDtoTokenID(int stateID, int tokID){
    stateIDtoTokenID.insert(std::make_pair(stateID, tokID));
}

DFA::~DFA() {}