#include "partial_valuation.h"

#include <algorithm>

PartialValuation::PartialValuation(unsigned nVars)
    :_values(nVars+1, ExtendedBool::Undefined), _currentLevel(0)
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

    /*Klauza je netacna u tekucoj parcijalnoj valuaciji ako za svaki literal klauze
      vazi da je u parcijalnoj valuaciji njemu suprotan literal.*/

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

    /* Klauza je jedinicna ako za svaki literal klauze osim jednog, parcijalna valuacija
       sadrzi njemu suprotan literal. Ovaj jedan je nedefinisan. */

    Literal undefinedLit = NullLiteral;
    int countUndefined = 0;

    /* Za svaki literal proveravamo da li je nedefinisan */
    for (Literal lit : c){

        ExtendedBool valueInClause = lit > 0 ? ExtendedBool::True : ExtendedBool::False;
        ExtendedBool valueInPValuation = _values[std::abs(lit)];

        if (valueInClause != valueInPValuation){

            if (valueInPValuation == ExtendedBool::Undefined){
                ++countUndefined;
                undefinedLit = lit;

                /* Ako naidjemo na jos jedan nedefinisan literal - klauza nije jedinicna */
                if (countUndefined > 1){
                    break;
                }
            }
        }
        else {
            return NullLiteral;
            // Jer za svaki literal iz klauze, pval mora da sadrzi njemu suprotan.
            // Ako su isti, sigurno nije jedinicna.
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
    //return countUndefined == 1 ? undefinedLit : NullLiteral;
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

        /*
        std::cout << "Stack before: " << std::endl;
        for (auto s : _stack){
            std::cout << s.first << " " << s.second << std::endl;
        }*/

        _stack.pop_back();

        /*
        std::cout << "Stack after: " << std::endl;
        for (auto s : _stack){
            std::cout << s.first << " " << s.second << std::endl;
        }*/
    }


    if (_stack.back().first == lit){
        _values[std::abs(_stack.back().first)] = ExtendedBool::Undefined;
        literals.push_back(_stack.back().first);
        _stack.pop_back();
        /*
        std::cout << "Stack after: " << std::endl;
        for (auto s : _stack){
            std::cout << s.first << " " << s.second << std::endl;
        }*/
    }

    if (_stack.empty()){
        _currentLevel = 0;
    }
    else {
        _currentLevel = _stack.back().second;
    }
}

void PartialValuation::lastAssertedLiteral(const Clause &c, Literal &lit) const {
    /*
        The last asserted literal of a clause c, is the literal from c that is in M (M - stack of partial valuation),
        such that no other literal from c comes after it in M .
    */

    for (auto it = _stack.rbegin(); it != _stack.rend(); it++){
        for (auto it2 = c.begin(); it2 != c.end(); it2++){
            if (it->first == *it2){
                lit = it->first;
                return;
            }
        }
    }
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
      throw std::logic_error{"Nepoznata vrednost dodeljena promenljivoj (nije ni True, ni False, ni Undefined)"};
    }
  }
  return out << " ]";
}

std::ostream &operator<<(std::ostream &out, const Clause &c){
    out << "[ ";
    for (auto it = c.cbegin(); it != c.cend(); it++){
        if (*it > 0){
            out << "p" << *it << " ";
        }
        else {
            out << "~p" << std::abs(*it) << " ";
        }
    }
    return out << " ]";
}
