#pragma once

#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <cstring>
#include <sstream>
#include <array>

#define N_STATE_CHAR_ID 6 //jumlah karakter maksimal dari kode state adalah 5, ditambah '\0', total 6 contohnya 'q1_1'

class State {
    public: 
        /**
         * @brief konstruktor default State, menghasilkan null state
         */
        State();
        /**
         * @brief Konstruktor elemen dari state "q0, .. ,q37" yang ada pada CONFIG
         * @param charID pointer karakter yang berisikan kode state "q0, .. ,q37"
         */
        State (const char* charID, int16_t newStateIdx, bool isFinalState);
        State (const State& other);
        State (State&& other) noexcept;
        bool isFinalState() const;
        int16_t getStateIdx() const;
        const char* getStateCharID() const;
        bool isNullState() const;
        bool compCharID(const char* otherCharID) const;
        ~State();

    private: 
        bool finState;
        int16_t stateIdx; // stateIdx adalah posisi/indeks dari State pada array state di kelas DFA 
        char stateCharID[N_STATE_CHAR_ID];

        /**
         * @brief fungsi bantu assign untuk assign di State.
         */
        void assignCharID(const char* newCharID);
};

class DFA {
    public :
        DFA();
        /**
         * @brief melakukan load transision termasuk token dari file konfigurasi
         * @note method ini menjadi satu-satunya cara untuk mengisi atribut kelas DFA persis setelah inisiasi.
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

        const State& getState() const;
        void next(unsigned char c);
        /**
         * @brief fungsi untuk melakukan reset state sehingga current state kembali ke start state
         */
        void resetState();

        ~DFA();
    
    private:
        int16_t currStateIdx;// jika bernilai -1 artinya null state

        std::vector<State> states;
        std::vector<std::array<int16_t, 128>> transTable;

        int16_t addUniqueState(const char* newCharID, bool newFinState);
        int16_t findStateIdx(const char* newCharID);
        void addStateWithIdx(int16_t idx, State nState);
        void setCurrentState(int16_t newStateIdx);
        void addTransition(int16_t state1, int input, int16_t state2);
};
