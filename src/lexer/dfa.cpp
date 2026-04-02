#include "dfa.hpp"

#include <stdexcept>

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

bool State::isFinalState() const { return finState; }
int16_t State::getStateIdx() const { return stateIdx; }
const char *State::getStateCharID() const { return stateCharID; }
void State::setFinalState(bool value) { finState = value; }

bool State::compCharID(const char *otherCharID) const
{
    return std::strcmp(stateCharID, otherCharID) == 0;
}

void State::assignCharID(const char *newCharID)
{
    std::strncpy(stateCharID, newCharID, sizeof(stateCharID) - 1);
    stateCharID[N_STATE_CHAR_ID - 1] = '\0';
}

bool State::isNullState() const { return stateIdx == -1; }
State::~State() {}

DFA::DFA() : currStateIdx(-1) {}

std::string DFA::toUpper(std::string s)
{
    for (char &c : s)
    {
        if (c >= 'a' && c <= 'z')
            c = static_cast<char>(c - 'a' + 'A');
    }
    return s;
}

void DFA::ensureStateCapacity(int16_t idx)
{
    if (idx < 0)
        return;
    while (static_cast<int>(states.size()) <= idx)
    {
        int16_t newIdx = static_cast<int16_t>(states.size());
        states.emplace_back("", newIdx, false);
    }
    while (static_cast<int>(transTable.size()) <= idx)
    {
        std::array<int16_t, MAX_ASCII_USED> row;
        row.fill(-1);
        transTable.push_back(row);
    }
}

int DFA::addOrGetTokenID(const std::string &name)
{
    auto it = tokenNameIDMapping.find(name);
    if (it != tokenNameIDMapping.end())
        return it->second;

    tokenTypes.emplace_back(name);
    int tokId = tokenTypes.back().get_type();
    tokenNameIDMapping[name] = tokId;
    return tokId;
}

void DFA::loadConfig(std::string path)
{
    std::ifstream fs(path);
    if (!fs.is_open())
    {
        throw std::runtime_error("Gagal membuka file config DFA: " + path);
    }

    std::string line;
    while (std::getline(fs, line))
    {
        if (line.empty())
            continue;

        std::istringstream stream(line);
        std::string kw;
        stream >> kw;
        if (kw.empty())
            continue;

        if (kw == "START")
        {
            std::string stateName;
            if (!(stream >> stateName))
                continue;
            ensureStateCapacity(0);
            states[0] = State(stateName.c_str(), 0, false);
            continue;
        }

        if (kw == "FINAL")
        {
            std::string tokName, stateName;
            if (!(stream >> tokName))
                continue;
            int tokId = addOrGetTokenID(tokName);
            if (stream >> stateName)
            {
                int stateIdx = addUniqueState(stateName.c_str(), true);
                states[stateIdx].setFinalState(true);
                addToStateIDtoTokenID(stateIdx, tokId);
            }
            continue;
        }

        if (kw == "FINALSET")
        {
            std::string tokName, stateName, lexeme;
            if (!(stream >> tokName >> stateName >> lexeme))
                continue;
            int tokId = addOrGetTokenID(tokName);
            keywordLexemeToTokenID[toUpper(lexeme)] = tokId;

            int stateIdx = addUniqueState(stateName.c_str(), true);
            states[stateIdx].setFinalState(true);
            if (stateIDtoTokenID.find(stateIdx) == stateIDtoTokenID.end())
            {
                addToStateIDtoTokenID(stateIdx, tokId);
            }
            continue;
        }

        std::string fromState, toState;
        if (!(stream >> fromState >> toState))
            continue;

        int inputASCII = std::stoi(kw);
        int fromIdx = addUniqueState(fromState.c_str(), false);
        int toIdx = addUniqueState(toState.c_str(), false);
        addTransition(fromIdx, inputASCII, toIdx);
    }

    if (states.empty())
    {
        throw std::runtime_error("Config DFA kosong atau tidak valid: " + path);
    }

    addOrGetTokenID("UNKNOWN");
}

void DFA::exportDFAConfig(std::string) const {}
void DFA::visualizeProcess(std::string) const {}

const State &DFA::getState() const
{
    static State nullstate;
    if (currStateIdx < 0 || currStateIdx >= static_cast<int16_t>(states.size()))
    {
        return nullstate;
    }
    return states[currStateIdx];
}

void DFA::next(unsigned char c)
{
    if (currStateIdx < 0 || currStateIdx >= static_cast<int16_t>(transTable.size()))
    {
        currStateIdx = -1;
        return;
    }
    currStateIdx = transTable[currStateIdx][c];
}

void DFA::resetState() { currStateIdx = 0; }

TokenType DFA::getCurrToken() const
{
    int tokId = getTokIDfromStateID(currStateIdx);
    for (const auto &tok : tokenTypes)
    {
        if (tok.get_type() == tokId)
            return tok;
    }
    return TokenType("UNKNOWN");
}

int DFA::getTokIDfromStateID(int stateID) const
{
    auto iter = stateIDtoTokenID.find(stateID);
    if (iter == stateIDtoTokenID.end())
        return getTokIDfromTokName("UNKNOWN");
    return iter->second;
}

int DFA::getTokIDfromTokName(std::string tokName) const
{
    auto iter = tokenNameIDMapping.find(tokName);
    if (iter == tokenNameIDMapping.end())
        return -1;
    return iter->second;
}

bool DFA::hasKeywordToken(const std::string &lexeme) const
{
    return keywordLexemeToTokenID.find(toUpper(lexeme)) != keywordLexemeToTokenID.end();
}

TokenType DFA::getKeywordToken(const std::string &lexeme) const
{
    auto it = keywordLexemeToTokenID.find(toUpper(lexeme));
    if (it == keywordLexemeToTokenID.end())
        return getCurrToken();

    for (const auto &tok : tokenTypes)
    {
        if (tok.get_type() == it->second)
            return tok;
    }
    return TokenType("UNKNOWN");
}

int DFA::addUniqueState(const char *newCharID, bool newFinState)
{
    int idx = findStateIdx(newCharID);
    if (idx >= 0)
    {
        if (newFinState)
            states[idx].setFinalState(true);
        return idx;
    }

    idx = static_cast<int>(states.size());
    ensureStateCapacity(static_cast<int16_t>(idx));
    states[idx] = State(newCharID, static_cast<int16_t>(idx), newFinState);
    return idx;
}

int DFA::findStateIdx(const char *newCharID) const
{
    for (int i = 0; i < static_cast<int>(states.size()); i++)
    {
        if (states[i].compCharID(newCharID))
            return i;
    }
    return -1;
}

void DFA::addStateWithIdx(int16_t idx, const State &nState)
{
    ensureStateCapacity(idx);
    states[idx] = nState;
}

void DFA::setCurrentState(int16_t newStateIdx) { currStateIdx = newStateIdx; }

void DFA::addTransition(int16_t state1, int input, int16_t state2)
{
    ensureStateCapacity(state1);
    ensureStateCapacity(state2);
    if (input >= 0 && input < MAX_ASCII_USED)
    {
        transTable[state1][input] = state2;
    }
}

void DFA::addToStateIDtoTokenID(int stateID, int tokID)
{
    stateIDtoTokenID[stateID] = tokID;
}

DFA::~DFA() {}
