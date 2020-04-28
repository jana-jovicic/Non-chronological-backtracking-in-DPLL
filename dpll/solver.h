#ifndef SOLVER_H
#define SOLVER_H

#include "partial_valuation.h"

#include <iostream>
#include <optional>
#include <map>

using OptionalPartialValuation = std::optional<PartialValuation>;

class Solver {
public:
    Solver(std::istream &dimacsStream);  // konstruktor od c++ stream-a
    Solver(const CNFFormula &formula);   // konstruktor od CNF formule

    // DPLL procedura
    OptionalPartialValuation solve();

private:


    bool checkConflict();
    bool checkUnit(Literal &lit, Clause &c);
    //bool chooseDecisionLiteral(Literal &lit);
    void initialAnalysis();

    void applyUnitPropagate(const Literal &lit, const Clause &c);
    void applyDecide(const Literal &lit);

    bool isUIP();
    void applyExplainUIP();
    void applyExplainEmpty();
    void applyLearn();
    void applyExplain(const Literal &lit);

    Clause invertClause(const Clause &c);
    Clause resolve(const Clause & c1, const Clause & c2, const Literal & lit);

    bool canBackjump();
    void applyBackjump(const Literal &lit);
    void getBackjumpLiteral(Literal &lit);

    //void restart();


    CNFFormula _formula;
    PartialValuation _valuation;
    Clause _conflict;
    std::map<Literal, Clause> _reason;
    int _cn; // number of literals from conflict clause on last decision level
    ExtendedBool _status;

};

#endif // SOLVER_H
