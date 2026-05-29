#pragma once

#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <sstream>
#include <array>
#include <unordered_map>
#include <string>
#include "token.hpp"

#define MAX_ASCII_USED 256

class State
{
public:
    State();
    State(const char *charID, int16_t newStateIdx, bool isFinalState);
    bool isFinalState() const;
    void setFinalState(bool isFinalState);
    int16_t getStateIdx() const;
    const char *getStateCharID() const;
    bool isNullState() const;
    bool compCharID(const char *otherCharID) const;
    ~State();

private:
    bool finState;
    int16_t stateIdx;
    std::string stateCharID;

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
    TokenType getTokenForState(int stateID) const;
    int getTokIDfromStateID(int stateID) const;
    int getTokIDfromTokName(std::string tokName) const;
    State getStateWithId(int stateId) const;
    bool hasKeywordToken(const std::string &lexeme) const;
    TokenType getKeywordToken(const std::string &lexeme) const;
    TokenType getTokenTypeFromTypeName(std::string typeName) const;

    ~DFA();

private:
    int16_t currStateIdx;
    static State nullState;
    static TokenType unknownToken;

    static bool isVisualized;
    static std::fstream visualFileStream;

    std::vector<State> states;
    std::vector<std::array<int16_t, MAX_ASCII_USED>> transTable;

    std::vector<TokenType> tokenTypes;
    std::unordered_map<std::string, int> tokenNameIDMapping;
    std::unordered_map<int, int> stateIDtoTokenID;
    std::unordered_map<std::string, int> keywordLexemeToTokenID;

    int addUniqueState(const char *newCharID, bool newFinState);
    int findStateIdx(const char *newCharID);
    void setCurrentState(int16_t newStateIdx);
    void addTransition(int16_t state1, int input, int16_t state2);

    void addTokenToTokenTypes(TokenType newTok);
    void addToTokenNameIDMapping(std::string name, int tokID);
    void addToStateIDtoTokenID(int stateID, int tokID);
    void addKeywordLexemeToTokenID(const std::string &lexeme, int tokID);
    int addOrGetTokenID(const std::string &name);
    static std::string toUpper(std::string text);

    void visualizedProccToFile(char c, State currState) const;
};