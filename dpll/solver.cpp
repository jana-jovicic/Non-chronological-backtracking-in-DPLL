#include "solver.h"

#include <string>
#include <sstream>
#include <stdexcept>
#include <iterator>
#include <algorithm>

#define DEBUG

Solver::Solver(std::istream &dimacsStream){

    std::string line;
    std::size_t firstNonSpaceIndex;

    while (std::getline(dimacsStream, line)){
        firstNonSpaceIndex = line.find_first_not_of(" \t\r\n");
        if (firstNonSpaceIndex != std::string::npos && line[firstNonSpaceIndex] != 'c'){
            break;
        }
    }

    /* Check if we read line of format 'p cnf varCount clauseCount' */
    if (line[firstNonSpaceIndex] != 'p'){
        throw std::runtime_error{"Input file isn't in DIMACS format. (p)"};
    }

    std::istringstream parser{line.substr(firstNonSpaceIndex+1, std::string::npos)};
    std::string tmp;
    if(!(parser >> tmp) || tmp != "cnf"){
        throw std::runtime_error{"Input file isn't in DIMACS format. (cnf)"};
    }

    unsigned varCount, clauseCount;
    if (!(parser >> varCount >> clauseCount)){
        throw std::runtime_error{"Input file isn't in DIMACS format. (varCount, clauseCount)"};
    }

    /* Read clauses line by line, ignoring comments and empty lines. */
    _formula.resize(clauseCount);
    _valuation.reset(varCount);
    int clauseIdx = 0;
    while(std::getline(dimacsStream, line)){
        firstNonSpaceIndex = line.find_first_not_of((" \t\r\n"));
        if (firstNonSpaceIndex != std::string::npos && line[firstNonSpaceIndex] != 'c'){
            parser.clear();
            parser.str(line);
            std::copy(std::istream_iterator<int>{parser}, {}, std::back_inserter(_formula[clauseIdx]));
            _formula[clauseIdx++].pop_back(); // pop zero from end of the line
        }
    }

    _cn = -1;

}

Solver::Solver(const CNFFormula &formula)
    : _formula(formula), _valuation(_formula.size()), _cn(-1)
{}

OptionalPartialValuation Solver::solve(){

    Literal lit;
    Clause reason;

    while (true){

        if (checkConflict()){
            initialAnalysis();

            if (canBackjump()){
                applyExplainUIP();
                applyLearn();

                Literal backjumpLiteral;
                getBackjumpLiteral(backjumpLiteral);
                applyBackjump(backjumpLiteral);

                _conflict.clear();

            }
            else {
                applyExplainEmpty();
                applyLearn();
                /* UNSAT */
                return {};
            }

        }

        /* If there is no conflict (and there is unit clause), we do exhaustive unit propagation.  */
        else if (checkUnit(lit, reason)){
            applyUnitPropagate(lit, reason);
        }

        /* If there is no unit clause, we choose a literal that will be propagated */
        else if ((lit = _valuation.firstUndefined())){
            applyDecide(lit);
        }

        else {
            /* SAT */
            return _valuation;
        }
    }
}


bool Solver::checkConflict() {

    for (Clause clauseInFormula : _formula) {
        if (_valuation.isClauseFalse(clauseInFormula)){
            _conflict = clauseInFormula;
#ifdef DEBUG
     std::cout << "Conflict clause: " << _conflict << std::endl;
#endif
            return true;
        }
    }
    return false;
}

bool Solver::checkUnit(Literal &lit, Clause &c){

    for (Clause clauseInFormula : _formula){
        if(_valuation.isClauseUnit(clauseInFormula, lit)){
            c = clauseInFormula;
            return true;
        }
    }

    return false;
}

void Solver::initialAnalysis() {
  _cn = _valuation.numberOfTopLevelLiterals(invertClause(_conflict));
}

bool Solver::canBackjump(){
    return _valuation.current_level() > 0;
}



void Solver::applyUnitPropagate(const Literal &lit, const Clause &c){
    _valuation.push(lit);
    _reason[std::abs(lit)] = c;
#ifdef DEBUG
    std::cout << "Literal " << (lit < 0 ? "~p" : "p" )<< std::abs(lit) << " propagated because of clause " << c << std::endl;
#endif
}

void Solver::applyDecide(const Literal &lit){
    _valuation.push(lit, true);
#ifdef DEBUG
    std::cout << "Literal " << (lit < 0 ? "~p" : "p" )<< std::abs(lit) << " decided" << std::endl;
#endif
}


bool Solver::isUIP(){
    /* first UIP - first unique implication point
       Using the firstUIP strategy, the learning process is terminated when the backjump clause contains
       exactly one literal from the current decision level.
    */
    return (_cn == -1 || _cn > 1) ? false : true;
}

void Solver::applyExplainUIP() {
    while (!isUIP()){
        Literal lit;
        _valuation.lastAssertedLiteral(invertClause(_conflict), lit);
        applyExplain(lit);
    }
}

void Solver::applyExplainEmpty() {
    while(!_conflict.empty()){
        Literal lit;
        _valuation.lastAssertedLiteral(invertClause(_conflict), lit);
        applyExplain(lit);
    }
}

void Solver::applyLearn(){
    _formula.push_back(_conflict);
#ifdef DEBUG
  std::cout << "Learned clause: " << _conflict << std::endl;
#endif
}

void Solver::applyExplain(const Literal &lit){
    Clause reason = _reason[std::abs(lit)];
    _conflict = resolve(_conflict, reason, lit);
    _cn = _valuation.numberOfTopLevelLiterals(invertClause(_conflict));
}


Clause Solver::invertClause(const Clause &c){
    Clause new_clause;
    std::for_each(c.cbegin(), c.cend(), [&new_clause](Literal lit){new_clause.push_back(-lit);});
    return new_clause;
}

Clause Solver::resolve(const Clause &c1, const Clause &c2, const Literal &lit) {
    Clause resolvent;

    /* Resolvent must contain all literals from first clause that are different from given literal (or its inverse). */
    std::copy_if(c1.cbegin(), c1.cend(), std::back_inserter(resolvent), [&lit](Literal c1Lit){return c1Lit != -lit && c1Lit != lit;});

    /* Resolvent must contain all literals from second clause that are different from given literal (or its inverse).
       Literals that exist in first clause are not copied (to avoid duplicates). */
    std::copy_if(c2.cbegin(), c2.cend(), std::back_inserter(resolvent), [&lit, c1](Literal c2Lit){
        return c2Lit != -lit && c2Lit != lit && (c1.end() == std::find(c1.begin(), c1.end(), c2Lit));});

#ifdef DEBUG
  std::cout << "Resolving clauses " << c1 << " and " << c2 << " into clause " << resolvent << std::endl;
#endif

    return resolvent;
}


void Solver::applyBackjump(const Literal &lit) {
    std::vector<Literal> literals;
    Literal literalForPropagation;
    _valuation.lastAssertedLiteral(invertClause(_conflict), literalForPropagation);
    _valuation.backjumpToLiteral(lit, literals);

#ifdef DEBUG
    std::cout << "Backjumping to literal " << (lit < 0 ? "~p" : "p" )<< std::abs(lit) << std::endl;
#endif

    for (Literal l : literals)
        _reason.erase(l);

    applyUnitPropagate(-literalForPropagation, _conflict);
}

void Solver::getBackjumpLiteral(Literal &lit) {
    _valuation.lastAssertedLiteral(invertClause(_conflict), lit);
    _conflict.erase(std::remove(_conflict.begin(), _conflict.end(), lit), _conflict.end());
    _valuation.lastAssertedLiteral(invertClause(_conflict), lit);
}

