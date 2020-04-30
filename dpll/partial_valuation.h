#ifndef PARTIAL_VALUATION_H
#define PARTIAL_VALUATION_H

#include <cstdint>
#include <vector>
#include <iostream>

#define NullLiteral (0)

enum class ExtendedBool: int8_t {
    False,
    True,
    Undefined
};

using Literal = int;
using Clause = std::vector<Literal>;
using CNFFormula = std::vector<Clause>;

class PartialValuation;

std::ostream& operator<<(std::ostream &out, const PartialValuation &pval);
std::ostream& operator<<(std::ostream &out, const Clause &c);

/**
 * @brief The PartialValuation class - represents partial valuation in which variables can be true, false or undefined.
 */

class PartialValuation {

public:
    PartialValuation(unsigned nVars = 0);

    /**
     * @brief push - Pushes the value of given literal lit to partial valuation.
     * @param decide - flag that indicates if literal is decided or not
     */
    void push(Literal lit, bool decide=false);


    /**
     * @brief isClauseFalse - Checks if clause c is false in current partal valuation.
     * Clause is false in current partial valuation if for every literal in clause,
     * partial valuation contains negation of that literal.
     * @return - true if clause is false, otherwise false
     */
    bool isClauseFalse(const Clause &c) const;


    /**
     * @brief isClauseUnit - Checks if clause c is unit.
     * Clause is unit if for every literal in clause except one, partial valuation contains negation of that literal.
     * That one literal is undefined.
     * @param lit - literal which is set to that udefined literal if clause is unit
     * @return - true if clause is unit, otherwise false
     */
    bool isClauseUnit(const Clause &c, Literal &lit) const;


    /**
     * @brief firstUndefined - Returns first undefined literal in valuation (literal with smallest index).
     * It is used for decide rule - first undefined literal is chosen as decided literal.
     */
    Literal firstUndefined() const;

    /**
     * @brief reset - Resets partial valuation. All variables are set to ExtendedBool::Undefinaed, and stack is emptied.
     * @param nVars - number of variables
     */
    void reset(unsigned nVars);


    /**
     * @brief current_level - Returns current decision level.
     */
    unsigned current_level() const;


    /**
     * @brief backjumpToLiteral - Backjums to given literal lit by deleting from stack all literals that were added after it.
     * Those literals are added to vector literals, so that they can be deleted from reason clause that lead to conflict.
     */
    void backjumpToLiteral(const Literal &lit, std::vector<Literal> &literals);


    /**
     * @brief lastAssertedLiteral - Sets literal lit to last asserted literal.
     * The last asserted literal of a clause c, is the literal from c that is on stack of partial valuation,
     * such that no other literal from c comes after it on stack.
     */
    void lastAssertedLiteral(const Clause &c, Literal &lit) const;


    /**
     * @brief numberOfTopLevelLiterals - Returns number of literals from partial vluation at current decision level
     * that are also present in clause c.
     */
    unsigned numberOfTopLevelLiterals(const Clause &c) const;

    friend std::ostream& operator<<(std::ostream &out, const PartialValuation &pval);

private:

    /**
     * @brief _values - values of variables in partial valuation
     */
    std::vector<ExtendedBool> _values;

    /**
     * @brief _stack - stack that holds information on order of assigning values to variables and decision level on which that occured.
     */
    std::vector<std::pair<Literal, unsigned>> _stack;

    /**
     * @brief _currentLevel - current decision level
     */
    unsigned _currentLevel;
};

#endif // PARTIAL_VALUATION_H
