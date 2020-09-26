#ifndef SOLVER_H
#define SOLVER_H

#include "partial_valuation.h"

#include <iostream>
#include <optional>
#include <map>

using OptionalPartialValuation = std::optional<PartialValuation>;

class Solver {
public:
    /**
     * @brief Solver - constructor from data that are given in DIMACS format
     * @param dimacsStream - input stream
     */
    Solver(std::istream &dimacsStream);

    /**
     * @brief Solver - constructor from CNF formula
     * @param formula - CNF formula for which satisfiability is checked
     */
    Solver(const CNFFormula &formula);


    /**
     * @brief solve - DPLL procedure
     * @return - partial valuaton if problem is SAT, or nothing if problem is UNSAT
     */
    OptionalPartialValuation solve();

private:

    /**
     * @brief checkConflict - checks if conflict clause exists
     * @return - true if there is a clause in which conflict occured, otherwise false
     */
    bool checkConflict();

    /**
     * @brief checkUnit - checks if unit clause exists
     * @param c - clause that is set to unit clause if it exists
     * @param lit - literal that is set to undefined literal of unit clause
     * @return - true if unit clause exists, otherwise false
     */
    bool checkUnit(Literal &lit, Clause &c);


    /**
     * @brief applyUnitPropagate - propagates unit literal of unit clause
     * @param lit - unit literal
     * @param c - unit clause that is reason for propagation of literal lit
     */
    void applyUnitPropagate(const Literal &lit, const Clause &c);

    /**
     * @brief applyDecide - applies decision rule
     * @param lit - literal that is decided
     */
    void applyDecide(const Literal &lit);


    /**
     * @brief isUIP - Checks if learning process should be terminated based on firstUIP (first unique implication point) strategy.
     * The learning process is terminated when the backjump clause contains exactly one literal from the current decision level.
     * @return - true if learning process should be terminated, otherwise false
     */
    bool isUIP();

    /**
     * @brief applyExplainUIP - Constructs backjump clause if conflict occured at a decision level other then zero.
     * It resolves out the last asserted literal of inverted conflict clause using the applyExplain function until conflict clause
     * satisfies the firstUIP condition.
     */
    void applyExplainUIP();

    /**
     * @brief applyExplainEmpty - Constructs backjump clause if conflict occured at a decision level zero.
     * It resolves out the last asserted literal of inverted conflict clause using the applyExplain function until conflict clause
     * becomes empty.
     */
    void applyExplainEmpty();

    /**
     * @brief applyExplain - Resolves out a literal lit by performing a single resolution step between conflict clause and
     * a clause that is the reason for propagation of literal lit.
     */
    void applyExplain(const Literal &lit);

    /**
     * @brief applyLearn - Adds the constructed conflict clause to the current set of clauses that are in formula.
     */
    void applyLearn();


    /**
     * @brief invertClause - inverts literals of clause c
     * @return - inverted clause
     */
    Clause invertClause(const Clause &c);

    /**
     * @brief resolve - Resolves out literal lit using clauses c1 and c2
     * @return - resolved clause
     */
    Clause resolve(const Clause & c1, const Clause & c2, const Literal & lit);

    /**
     * @brief canBackjump - Checks if backjump can be applied.
     */
    bool canBackjump();

    /**
     * @brief applyBackjump - Backjumps to given literal lit.
     */
    void applyBackjump(const Literal &lit);

    /**
     * @brief applyBackjumpToStart - Backjumps to start
     */
    void applyBackjumpToStart();

    /**
     * @brief getBackjumpLiteral - Finds literal to which needs to be backjumped and saves it in parameter lit
     */
    void getBackjumpLiteral(Literal &lit, bool &restart);

    void restart();


    CNFFormula _formula;
    PartialValuation _valuation;
    Clause _conflict;
    std::map<Literal, Clause> _reason; /* maps the literal and clause that is a reason for its poropagation */
    int _nConflictTopLevelLiteras; /* number of literals from conflict clause on last decision level */

};

#endif // SOLVER_H
