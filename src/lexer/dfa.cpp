#include "dfa.hpp"

#include <cctype>
#include <stdexcept>

// State-lihat penjelasan pada dfa.hpp

State::State()
{
    assignCharID("q_stale");
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

void State::setFinalState(bool isFinalState)
{
    finState = isFinalState;
}

int16_t State::getStateIdx() const
{
    return stateIdx;
}

const char *State::getStateCharID() const
{
    return stateCharID.c_str();
}

bool State::compCharID(const char *otherCharID) const
{
    return stateCharID == otherCharID;
}

void State::assignCharID(const char *newCharID)
{
    stateCharID = newCharID;
}

bool State::isNullState() const
{
    if (stateIdx == -1)
        return true;
    return false;
}

State::~State() {}

// DFA-Lihat penjelasan pada dfa.hpp
TokenType DFA::unknownToken = TokenType("UNKNOWN");
bool DFA::isVisualized = false;
State DFA::nullState = State();
std::fstream DFA::visualFileStream;

DFA::DFA() : currStateIdx(0)
{
}

void DFA::loadConfig(std::string path)
{
    std::fstream fs;
    fs.open(path, std::ios::in);
    if (!fs)
    {
        throw std::runtime_error("Gagal membuka file config DFA: " + path);
    }

    std::string line;
    while (std::getline(fs, line))
    {
        std::istringstream stream(line);
        std::string kw;
        if (!(stream >> kw))
        {
            continue;
        }

        if (kw == "START")
        {
            std::string startState;
            if (stream >> startState)
            {
                addUniqueState(startState.c_str(), false);
                resetState();
            }
            continue;
        }

        if (kw == "FINAL")
        {
            std::string tokenName;
            std::string stateName;
            if ((stream >> tokenName) && (stream >> stateName))
            {
                const int tokId = addOrGetTokenID(tokenName);
                const int stateIdx = addUniqueState(stateName.c_str(), true);
                addToStateIDtoTokenID(stateIdx, tokId);
            }
            continue;
        }

        std::string fromState;
        std::string toState;
        if (stream >> fromState >> toState)
        {
            const int inputASCII = std::stoi(kw);
            int16_t fromIdx = findStateIdx(fromState.c_str());
            if (fromIdx < 0)
            {
                fromIdx = addUniqueState(fromState.c_str(), false);
            }

            int16_t toIdx = findStateIdx(toState.c_str());
            if (toIdx < 0)
            {
                toIdx = addUniqueState(toState.c_str(), false);
            }

            addTransition(fromIdx, inputASCII, toIdx);
        }
    }

    const int stringTokId = getTokIDfromTokName("STRING");
    const int stringContinuationState = findStateIdx("q4_2");
    if (stringTokId >= 0 && stringContinuationState >= 0 &&
        stateIDtoTokenID.find(stringContinuationState) == stateIDtoTokenID.end())
    {
        states[stringContinuationState].setFinalState(true);
        addToStateIDtoTokenID(stringContinuationState, stringTokId);
    }

    fs.close();
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
    if (!visualFileStream.good())
    {
        std::cout << "Gagal menulis ke dalam visualFileStream" << std::endl;
        return;
    }
}

const State &DFA::getState() const
{
    if (currStateIdx < 0 || currStateIdx >= static_cast<int>(states.size()))
    {
        return nullState; // mengembalikan null state
    }
    return states[currStateIdx];
}

void DFA::next(unsigned char c)
{
    if (currStateIdx == -1)
    {
        // std::cout << "Gagal melakukan pergantian state, DFA belum terinisiasi\n";
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
    else if (newFinState)
    {
        states[idx].setFinalState(true);
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
        // std::cout << "Gagal mengambil token, DFA masih belum diinisialisasi\n";
        return unknownToken;
    }
    int tokId = getTokIDfromStateID(currStateIdx);
    if (tokId < 0){
        // std::cout << "Pasangan dari state tidak ditemukan\n";
        return unknownToken;
    }
    return tokenTypes[tokId];
}

TokenType DFA::getTokenForState(int stateID) const
{
    const int tokId = getTokIDfromStateID(stateID);
    if (tokId < 0 || tokId >= static_cast<int>(tokenTypes.size()))
    {
        return unknownToken;
    }
    return tokenTypes[tokId];
}

int DFA::getTokIDfromStateID(int stateID) const
{
    if (stateID < 0)
    {
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
    auto iter = tokenNameIDMapping.find(tokName);
    if (iter != tokenNameIDMapping.end())
    {
        return iter->second;
    }

    auto unknownIter = tokenNameIDMapping.find("unknown");
    if (unknownIter != tokenNameIDMapping.end())
    {
        return unknownIter->second;
    }

    return -1;
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

void DFA::addKeywordLexemeToTokenID(const std::string &lexeme, int tokID)
{
    keywordLexemeToTokenID[toUpper(lexeme)] = tokID;
}

int DFA::addOrGetTokenID(const std::string &name)
{
    auto it = tokenNameIDMapping.find(name);
    if (it != tokenNameIDMapping.end())
    {
        return it->second;
    }

    TokenType newTok(name);
    const int tokId = static_cast<int>(tokenTypes.size());
    addTokenToTokenTypes(newTok);
    addToTokenNameIDMapping(name, tokId);
    return tokId;
}

std::string DFA::toUpper(std::string text)
{
    for (char &ch : text)
    {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return text;
}

bool DFA::hasKeywordToken(const std::string &lexeme) const
{
    return keywordLexemeToTokenID.find(toUpper(lexeme)) != keywordLexemeToTokenID.end();
}

TokenType DFA::getKeywordToken(const std::string &lexeme) const
{
    auto it = keywordLexemeToTokenID.find(toUpper(lexeme));
    if (it == keywordLexemeToTokenID.end())
    {
        return unknownToken;
    }

    const int tokId = it->second;
    if (tokId < 0 || tokId >= static_cast<int>(tokenTypes.size()))
    {
        return unknownToken;
    }

    return tokenTypes[tokId];
}

TokenType DFA::getTokenTypeFromTypeName(std::string typeName) const
{
    auto it = tokenNameIDMapping.find(typeName);
    if (it == tokenNameIDMapping.end())
    {
        return unknownToken;
    }

    const int tokId = it->second;
    if (tokId < 0 || tokId >= static_cast<int>(tokenTypes.size()))
    {
        return unknownToken;
    }

    return tokenTypes[tokId];
}

State DFA::getStateWithId(int stateId) const
{
    if (stateId < 0 || stateId >= static_cast<int>(states.size()))
    {
        return nullState;
    }
    return states[stateId];
}

DFA::~DFA() {}