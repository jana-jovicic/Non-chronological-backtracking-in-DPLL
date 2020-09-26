#include "partial_valuation.h"

#include <algorithm>

PartialValuation::PartialValuation(unsigned nVars)
    :_values(nVars, ExtendedBool::Undefined), _currentLevel(0)
{
    _stack.reserve(nVars);
}

void PartialValuation::push(Literal lit, bool decide) {

    _values[std::abs(lit)] = lit > 0 ? ExtendedBool::True : ExtendedBool::False;

    if (decide){
        _currentLevel++;
    }
    _stack.push_back(std::make_pair(lit, _currentLevel));
}

bool PartialValuation::isClauseFalse(const Clause &c) const {

    /*
        Clause is false in current partial valuation if for every literal in clause,
        partial valuation contains negation of that literal.
    */

    for (Literal lit : c){

        ExtendedBool variableInClause = lit > 0 ? ExtendedBool::True : ExtendedBool::False;
        ExtendedBool variableInPValuation = _values[std::abs(lit)];

        if( variableInClause == variableInPValuation || variableInPValuation == ExtendedBool::Undefined){
            return false;
        }
    }
    return true;
}

bool PartialValuation::isClauseUnit(const Clause &c, Literal &lit) const {

    /*
        Clause is unit if for every literal in clause except one,
        partial valuation contains negation of that literal.
        That one literal is undefined.
    */

    Literal undefinedLit = NullLiteral;
    int countUndefined = 0;

    /* For every literal we check if it is undefined. */
    for (Literal lit : c){

        ExtendedBool valueInClause = lit > 0 ? ExtendedBool::True : ExtendedBool::False;
        ExtendedBool valueInPValuation = _values[std::abs(lit)];

        if (valueInClause != valueInPValuation){

            if (valueInPValuation == ExtendedBool::Undefined){
                ++countUndefined;
                undefinedLit = lit;

                /* If we find another undefined literal - clause is not unit */
                if (countUndefined > 1){
                    break;
                }
            }
        }
        else {
            return false;
            /* Returns false, because partial valuation must contain literal that is negation of literal in clause or undefined literal.
               If literals in partial valuation and clause are equivalent, clause is not unit. */

        }
    }

    if (countUndefined == 1){
        lit = undefinedLit;
        return true;
    }
    else {
        lit = NullLiteral;
        return false;
    }
}

Literal PartialValuation::firstUndefined() const {
    auto it = std::find(_values.cbegin()+1, _values.cend(), ExtendedBool::Undefined);
    return it != _values.cend() ? it-_values.cbegin() : NullLiteral;
}

void PartialValuation::reset(unsigned nVars) {
    _values.resize(nVars + 1);
    std::fill(_values.begin(), _values.end(), ExtendedBool::Undefined);
    _stack.clear();
    _stack.reserve(nVars);
}

unsigned PartialValuation::current_level() const {
    return _currentLevel;
}

void PartialValuation::backjumpToLiteral(const Literal &lit, std::vector<Literal> &literals) {

    while (_stack.back().first != lit && !_stack.empty()){
        _values[std::abs(_stack.back().first)] = ExtendedBool::Undefined;
        literals.push_back(_stack.back().first);
        _stack.pop_back();

    }


    if (_stack.empty()){
        _currentLevel = 0;
    }
    else {
        _currentLevel = _stack.back().second;
    }
}

void PartialValuation::lastAssertedLiteral(const Clause &c, Literal &lit, bool &empty) const {
    /*
        The last asserted literal of a clause c, is the literal from c that is on stack of partial valuation,
        such that no other literal from c comes after it in stack.
    */
    empty = false;
    for (auto it = _stack.rbegin(); it != _stack.rend(); it++){
        for (auto it2 = c.begin(); it2 != c.end(); it2++){
            if (it->first == *it2){
                lit = it->first;
                return;
            }
        }
    }
    empty = true;
}

unsigned PartialValuation::numberOfTopLevelLiterals(const Clause &c) const {
    unsigned count = 0;

    for (auto it = _stack.rbegin(); it != _stack.rend(); it++){
        if (it->second < _currentLevel){
            return count;
        }
        else {
            /* if clause c contains current literal */
            for (auto it2 = c.begin(); it2 != c.end(); it2++){
                if (it->first == *it2){
                    count++;
                    break;
                }
            }
        }
    }
    return count;
}

void PartialValuation::clear(){
    _stack.clear();
    std::fill(_values.begin(), _values.end(), ExtendedBool::Undefined);
    _currentLevel = 0;
}

std::ostream &operator<<(std::ostream &out, const PartialValuation &pval){

  out << "[ ";
  for (std::size_t i = 1; i < pval._values.size(); ++i)
  {
    if (pval._values[i] == ExtendedBool::True)
    {
      out << 'p' << i << ' ';
    }
    else if (pval._values[i] == ExtendedBool::False)
    {
      out << "~p" << i << ' ';
    }
    else if (pval._values[i] == ExtendedBool::Undefined)
    {
      out << 'u' << i << ' ';
    }
    else
    {
      throw std::logic_error{"Unknow value assigned to variable (nor True, nor False, nor Undefined)"};
    }
  }
  return out << " ]";
}

std::ostream &operator<<(std::ostream &out, const Clause &c){
    out << "[ ";
    for (Literal lit : c){
        out << (lit > 0 ? "p" : "~p") << std::abs(lit) << " ";
    }
    return out << " ]";
}
