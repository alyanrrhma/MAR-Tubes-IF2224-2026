#pragma once

#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <cstring>
#include <sstream>
#include <array>
#include <unordered_map>
#include <string>
#include "token.hpp"

#define N_STATE_CHAR_ID 8
#define MAX_ASCII_USED 128

class State
{
public:
    State();
    State(const char *charID, int16_t newStateIdx, bool isFinalState);
    bool isFinalState() const;
    int16_t getStateIdx() const;
    const char *getStateCharID() const;
    bool isNullState() const;
    bool compCharID(const char *otherCharID) const;
    void setFinalState(bool value);
    ~State();

private:
    bool finState;
    int16_t stateIdx;
    char stateCharID[N_STATE_CHAR_ID];
    void assignCharID(const char *newCharID);
};

class DFA
{
public:
    DFA();
    void loadConfig(std::string path);
    void visualizeProcess(std::string path) const;
    void exportDFAConfig(std::string path) const;
    const State &getState() const;
    void next(unsigned char c);
    void resetState();
    TokenType getCurrToken() const;
    int getTokIDfromStateID(int stateID) const;
    int getTokIDfromTokName(std::string tokName) const;
    bool hasKeywordToken(const std::string &lexeme) const;
    TokenType getKeywordToken(const std::string &lexeme) const;
    ~DFA();

private:
    int16_t currStateIdx;
    std::vector<State> states;
    std::vector<std::array<int16_t, MAX_ASCII_USED>> transTable;
    std::vector<TokenType> tokenTypes;
    std::unordered_map<std::string, int> tokenNameIDMapping;
    std::unordered_map<int, int> stateIDtoTokenID;
    std::unordered_map<std::string, int> keywordLexemeToTokenID;

    int addUniqueState(const char *newCharID, bool newFinState);
    int findStateIdx(const char *newCharID) const;
    void addStateWithIdx(int16_t idx, const State &nState);
    void setCurrentState(int16_t newStateIdx);
    void addTransition(int16_t state1, int input, int16_t state2);
    void ensureStateCapacity(int16_t idx);
    int addOrGetTokenID(const std::string &name);
    void addToStateIDtoTokenID(int stateID, int tokID);
    static std::string toUpper(std::string s);
};
