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
TokenType DFA::unknownToken = TokenType("unknown");
bool DFA::isVisualized = false;
State DFA::nullState = State();
std::fstream DFA::visualFileStream;

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
        std::string line, kw = "", tk1 = "", tk2 = "";
        bool ckKw, ckTk1, ckTk2;
        while (std::getline(fs, line))
        {
            std::istringstream stream(line);
            ckKw = static_cast<bool>(stream >> kw);
            ckTk1 = static_cast<bool>(stream >> tk1);
            ckTk2 = static_cast<bool>(stream >> tk2);

            if (ckKw)
            {
                if (kw == "START")
                {
                    if (ckTk1)
                    {
                        addUniqueState(tk1.c_str(), false);
                    }
                }
                else if (kw == "FINAL")
                {
                    if (ckTk1 && ckTk2)
                    {
                        int tokId;
                        auto it = tokenNameIDMapping.find(tk1);
                        if (it == tokenNameIDMapping.end())
                        {
                            TokenType newTok(tk1);
                            tokId = newTok.get_type();
                            tokenTypes.push_back(newTok);
                            addToTokenNameIDMapping(tk1, tokId);
                        }
                        else
                        {
                            tokId = it->second;
                        }
                        int stateIdx = addUniqueState(tk2.c_str(), true);

                        addToStateIDtoTokenID(stateIdx, tokId);
                    }
                }
                else
                {
                    // parser untuk format transisi
                    if (ckTk1 && ckTk2)
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
        fs.close();
    }
}

void DFA::exportDFAConfig(std::string path) const
{
    std::ofstream fs(path);
    if (!fs)
    {
        std::cout << "Gagal membuka file output untuk export DFA.\n";
        return;
    }

    // START
    if (!states.empty())
    {
        fs << "START " << states[0].getStateCharID() << "\n\n";
    }
    else
    {
        std::cout << "DFA kosong, tidak ada state untuk diexport.\n";
        return;
    }

    // FINAL
    // stateIDtoTokenID : stateIdx -> tokenId
    // tokenTypes[tokenId] diasumsikan valid dan posisinya sesuai id token
    for (const auto& entry : stateIDtoTokenID)
    {
        int stateIdx = entry.first;
        int tokenId = entry.second;

        if (stateIdx < 0 || stateIdx >= static_cast<int>(states.size()))
        {
            continue;
        }

        if (tokenId < 0 || tokenId >= static_cast<int>(tokenTypes.size()))
        {
            continue;
        }

        fs << "FINAL "
           << tokenTypes[tokenId].get_name() << " "
           << states[stateIdx].getStateCharID() << "\n";
    }

    fs << "\n";

    // TRANSITIONS
    for (int from = 0; from < static_cast<int>(transTable.size()); ++from)
    {
        for (int ascii = 0; ascii < MAX_ASCII_USED; ++ascii)
        {
            int16_t to = transTable[from][ascii];
            if (to != -1)
            {
                if (to >= 0 && to < static_cast<int>(states.size()))
                {
                    fs << ascii << " "
                       << states[from].getStateCharID() << " "
                       << states[to].getStateCharID() << "\n";
                }
            }
        }
    }

    fs.close();
}

void DFA::visualizeProcess(std::string path) const
{
    if (!isVisualized)
    {
        visualFileStream.open(path, std::ios::out);
        if (!visualFileStream)
        {
            std::cout << "Gagal membukan file visual\n";
        }
        else
        {
            isVisualized = true;
        }
    }
}

void DFA::visualizedProccToFile(char c, State currState) const
{
    if (isVisualized)
    {
        visualFileStream << c << " ==> " << currState.getStateCharID() << std::endl;
    }
    if (visualFileStream.good())
    {
        std::cout << "Gagal menulis ke dalam visualFileStream" << std::endl;
        return;
    }
}

const State &DFA::getState() const
{
    if (currStateIdx < 0 || currStateIdx >= static_cast<int>(states.size()))
    {
        std::cout << "DFA berada dalam current state tidak valid. \n";
        return nullState; // mengembalikan null state
    }
    return states[currStateIdx];
}

void DFA::next(unsigned char c)
{
    if (currStateIdx == -1)
    {
        std::cout << "Gagal melakukan pergantian state, DFA belum terinisiasi\n";
        return;
    }
    int16_t newStateIdx = transTable[currStateIdx][c];
    // menghasilkan -1 jika state tidak ditemukan, artinya pada saat demikian state menjadi tidak valid atau null;
    currStateIdx = newStateIdx;
    if (isVisualized){
        visualizedProccToFile(c, getState());
    }
}

void DFA::resetState()
{
    currStateIdx = 0; // konvensi bahwa stateIdx 0 adalah start state;
}

int DFA::addUniqueState(const char *newCharID, bool newFinState)
{
    int idx;

    idx = findStateIdx(newCharID);
    if (idx < 0)
    {
        idx = states.size();
        states.push_back(State(newCharID, states.size(), newFinState));
    }
    return idx;
}

int DFA::findStateIdx(const char *newCharID)
{
    bool found = false;
    int addedStateIdx = states.size();
    for (int i = 0; i < static_cast<int>(states.size()); i++)
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

TokenType DFA::getCurrToken() const
{
    if (currStateIdx == -1)
    {
        std::cout << "Gagal mengambil token, DFA masih belum diinisialisasi\n";
        return unknownToken;
    }
    int tokId = getTokIDfromStateID(currStateIdx);
    if (tokId < 0){
        std::cout << "Pasangan dari state tidak ditemukan\n";
        return unknownToken;
    }
    return tokenTypes[tokId];
}

int DFA::getTokIDfromStateID(int stateID) const
{
    if (currStateIdx == -1)
    {
        std::cout << "Gagal mengambil token, DFA masih belum diinisialisasi\n";
        return -1;
    }
    int tokId;
    auto iter = stateIDtoTokenID.find(stateID);
    if (iter == stateIDtoTokenID.end())
    {
        tokId = -1;
    }
    else
    {
        tokId = iter->second;
    }
    return tokId;
}

int DFA::getTokIDfromTokName(std::string tokName) const
{
    if (currStateIdx == -1)
    {
        std::cout << "Gagal mengambil token, DFA masih belum diinisialisasi\n";
        return -1;
    }

    int tokId;
    auto iter = tokenNameIDMapping.find(tokName);
    if (iter == tokenNameIDMapping.end())
    {
        tokId = getTokIDfromTokName("unknown");
    }
    else
    {
        tokId = iter->second;
    }
    return tokId;
}

void DFA::setCurrentState(int16_t newStateIdx)
{
    currStateIdx = newStateIdx;
}

void DFA::addTransition(int16_t state1, int input, int16_t state2)
{
    while (transTable.size() <= static_cast<size_t>(state1))
    {
        std::array<int16_t, MAX_ASCII_USED> row;
        row.fill(-1);
        transTable.push_back(row);
    }

    transTable[state1][input] = state2;
}

void DFA::addTokenToTokenTypes(TokenType newTok)
{
    tokenTypes.push_back(newTok);
}

void DFA::addToTokenNameIDMapping(std::string name, int tokId)
{
    tokenNameIDMapping[name] = tokId;
}

void DFA::addToStateIDtoTokenID(int stateID, int tokID)
{
    stateIDtoTokenID[stateID] = tokID;
}

DFA::~DFA() {}