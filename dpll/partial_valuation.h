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

class PartialValuation {

public:
    PartialValuation(unsigned nVars = 0);

    void push(Literal lit, bool decide=false);

    bool isClauseFalse(const Clause &c) const;

    bool isClauseUnit(const Clause &c, Literal &lit) const;

    Literal firstUndefined() const;

    void reset(unsigned nVars);

    unsigned current_level() const;

    void backjumpToLiteral(const Literal &lit, std::vector<Literal> &literals);

    void lastAssertedLiteral(const Clause &c, Literal &lit) const;

    unsigned numberOfTopLevelLiterals(const Clause &c) const;

    friend std::ostream& operator<<(std::ostream &out, const PartialValuation &pval);

private:
    std::vector<ExtendedBool> _values;
    std::vector<std::pair<Literal, unsigned>> _stack;
    unsigned _currentLevel;
};

#endif // PARTIAL_VALUATION_H
