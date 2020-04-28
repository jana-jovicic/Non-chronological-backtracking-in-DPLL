#include <QCoreApplication>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include "solver.h"


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (argc != 2){
        throw std::runtime_error{"Add dimacs file as argument"};
    }

    std::ifstream dimacsStream(argv[1]);
    if (!dimacsStream.is_open()){
        throw std::runtime_error{"Bad path to dimacs file. (main)"};
    }

    Solver s{dimacsStream};

    OptionalPartialValuation solution = s.solve();
    if (solution)
    {
        std::cout << "SAT" << std::endl;
        std::cout << solution.value() << std::endl;
    }
    else
    {
        std::cout << "UNSAT" << std::endl;
    }


    return a.exec();
}

