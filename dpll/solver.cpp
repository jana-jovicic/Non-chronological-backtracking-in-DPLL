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

    /* Proveravamo da smo procitali liniju 'p cnf brPromenljivih brKlauza' */
    if (line[firstNonSpaceIndex] != 'p'){
        throw std::runtime_error{"Pogresan format ulaza iz DIMACS stream-a. (p)"};
    }

    //std::istringstream implements input operations on string based streams
    std::istringstream parser{line.substr(firstNonSpaceIndex+1, std::string::npos)};
    std::string tmp;
    if(!(parser >> tmp) || tmp != "cnf"){
        throw std::runtime_error{"Pogresan format ulaza iz DIMACS stream-a. (cnf)"};
    }

    unsigned varCount, clauseCount;
    if (!(parser >> varCount >> clauseCount)){
        throw std::runtime_error{"Pogresan format ulaza iz DIMACS stream-a. (varCount, clauseCount)"};
    }

    /* Citamo klauze linije po liniju preskacuci komentare i prazne linije */
    _formula.resize(clauseCount);
    _valuation.reset(varCount);
    int clauseIdx = 0;
    while(std::getline(dimacsStream, line)){
        firstNonSpaceIndex = line.find_first_not_of((" \t\r\n"));
        if (firstNonSpaceIndex != std::string::npos && line[firstNonSpaceIndex] != 'c'){
            parser.clear();
            parser.str(line);
            std::copy(std::istream_iterator<int>{parser}, {}, std::back_inserter(_formula[clauseIdx]));
            _formula[clauseIdx++].pop_back(); /* izbacujemo nulu sa kraja linije */
        }
    }

    _cn = -1;
    _status = ExtendedBool::Undefined;

    /*
    for (auto clause : _formula){
        for (auto lit : clause){
            std::cout << lit << "\n";
        }
        std::cout << "*****\n";
    }*/
}

Solver::Solver(const CNFFormula &formula)
    : _formula(formula), _valuation(_formula.size()), _cn(-1), _status(ExtendedBool::Undefined)
{}

OptionalPartialValuation Solver::solve(){

    Literal lit;
    Clause reason;

    while (_status == ExtendedBool::Undefined){

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
                _status = ExtendedBool::False;
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
            _status = ExtendedBool::True;
            return _valuation;
        }
    }
}


bool Solver::checkConflict() {

    for (auto it = _formula.begin(); it != _formula.end(); it++) {
        if (_valuation.isClauseFalse(*it)){
            _conflict = *it;
            return true;
        }
    }
    return false;
}

bool Solver::checkUnit(Literal &lit, Clause &c){

    for (auto it = _formula.begin(); it != _formula.end(); it++){
        if (_valuation.isClauseUnit(*it, lit)){
            c = *it;
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
  std::cout << "Literal p" << lit << " propagated because of clause " << c << std::endl;
#endif
}

void Solver::applyDecide(const Literal &lit){
    _valuation.push(lit, true);
#ifdef DEBUG
  std::cout << "Literal p" << lit << " decided" << std::endl;
#endif
}


bool Solver::isUIP(){
    /* first UIP - first unique implication point
       Using the firstUIP strategy, the learning process is terminated when the backjump clause contains
       exactly one literal from the current decision level.
    */
    if (_cn == -1 || _cn > 1){
        return false;
    }
    else {
        return true;
    }
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
    for (auto it = c.begin(); it != c.end(); it++) {
        new_clause.push_back(-(*it));
    }
    return new_clause;
}

Clause Solver::resolve(const Clause &c1, const Clause &c2, const Literal &lit) {
    Clause resolvent;

    for (auto it = c1.begin(); it != c1.end(); it++){
        if (*it != -lit){
            resolvent.push_back(*it);
        }
    }

    for (auto it = c2.begin(); it != c2.end(); it++){
        if (*it != lit && c1.end() == std::find(c1.begin(), c1.end(), *it)){
            resolvent.push_back(*it);
        }
    }

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

    for (Literal l : literals)
        _reason.erase(l);

    applyUnitPropagate(-literalForPropagation, _conflict);
}

void Solver::getBackjumpLiteral(Literal &lit) {
    _valuation.lastAssertedLiteral(invertClause(_conflict), lit);
    Clause tmp;
    //std::copy(_conflict.begin(), _conflict.end(), std::back_inserter(tmp));
    //tmp.erase(std::remove(tmp.begin(), tmp.end(), lit), tmp.end());

    for (auto it = _conflict.begin(); it != _conflict.end(); it++) {
        if (*it != -lit) {
          tmp.push_back(*it);
        }
     }
    _valuation.lastAssertedLiteral(invertClause(tmp), lit);
}

