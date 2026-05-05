#include "../../src/parser/parse_tree.hpp"
#include <iostream>

/*
Tester apakah parse tree sudah berjalan atau tidak. Boleh dihapus nanti Ketikkan perintah :

g++ -std=c++17 -Wall -Wextra -g test/milestone2/parse_tree_test.cpp src/parser/parse_tree.cpp src/lexer/token.cpp -o test/milestone2/parse_tree_test.exe

lalu :

./test/milestone2/parse_tree_test.exe
*/

int main() {
    using namespace parse_tree;

    NodePtr root = makeNonTerminal(NonTerminal::Program);
    Node* header = root->addChild(createNonTerminal(NonTerminal::ProgramHeader));
    header->addChild(createTerminal("programsy"));
    header->addChild(createTerminal("ident", "Hello"));
    header->addChild(createTerminal("semicolon"));

    Node* statement = root->addChild(createNonTerminal(NonTerminal::Statement));
    Node* assignment = statement->addChild(createNonTerminal(NonTerminal::AssignmentStatement));
    assignment->addChild(createTerminal("ident", "a"));
    assignment->addChild(createTerminal("becomes"));
    assignment->addChild(createNonTerminal(NonTerminal::Expression));

    Node* empty = root->addChild(createNonTerminal(NonTerminal::Statement));
    empty->addChild(createNonTerminal(NonTerminal::Empty));

    root->addChild(createError("unexpected unknown token"));

    printTree(root.get(), std::cout);
    return 0;
}

/*
Output yang diharapkan 
<program>
  <program-header>
    programsy
    ident (Hello)
    semicolon
  <statement>
    <assignment-statement>
      ident (a)
      becomes
      <expression>
  <statement>
    <empty>
  <error: unexpected unknown token>
*/
