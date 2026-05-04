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
    /**
     * @brief konstruktor default State, menghasilkan null state
     */
    State();
    /**
     * @brief Konstruktor elemen dari state "q0, .. ,q37" yang ada pada CONFIG
     * @param charID pointer karakter yang berisikan kode state "q0, .. ,q37"
     */
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
    char stateCharID[N_STATE_CHAR_ID];

    /**
     * @brief fungsi bantu assign untuk assign di State.
     */
    void assignCharID(const char *newCharID);
};

class DFA
{
public:
    DFA();
    /**
     * @brief melakukan load transision termasuk token dari file konfigurasi
     * @note method ini menjadi satu-satunya cara untuk mengisi atribut kelas DFA persis setelah inisiasi.
     * @note START harus selalu di baris pertama file.
     */
    void loadConfig(std::string path);

    /**
     * @brief metode yang dibuat dengan tujuan debugging, mem-visualisasiikan proses perpindahan state dan scanner pada DFA.
     * @note hasil dari visualisasi disimpan pada path input.
     */
    void visualizeProcess(std::string path) const;
    /**
     * @brief export DFA config for debugging purpose.
     */
    void exportDFAConfig(std::string path) const;

    const State &getState() const;

    /**
     * @brief next tidak menghasilkan karakter karena DFA hanya bertanggung jawab terhadap perubahasn state saja.
     * @note penyimpanan value dilakukan di luar DFA.
     */
    void next(unsigned char c);
    /**
     * @brief fungsi untuk melakukan reset state sehingga current state kembali ke start state
     */
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
    std::vector<std::array<int16_t, MAX_ASCII_USED> > transTable;

    // Penyimpanan TokenType
    /**
     * @brief tempat menyimpan kumpulan dari TokenType
     * @note id dari TokenType sama dengna index pada vector tokenTypes
     */
    std::vector<TokenType> tokenTypes;

    /**
     * @brief tokenNameIDMapping memetakan nama ke id dari tokenType
     * @note tujuannya adalah untuk mencegah pembuatan token yang sama dua kali.
     */
    std::unordered_map<std::string, int> tokenNameIDMapping;

    /**
     * @brief stateIDtoTokenID merupakan pemetaan antara stateID ke Token ID
     * @note tujuannya adalah supaya setiap (final) state yang terhubung dengan token ID dapat mengakses TokenTypes secara tidak langsung melalui StateID-nya
     */
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
