#pragma once

#include "common.h"

#include "FormulaAST.h"

#include <memory>
#include <variant>

// Формула с бинарными и унарными операциями
class FormulaInterface {
public:
    using Value = std::variant<double, FormulaError>;

    virtual ~FormulaInterface() = default;

    // Возвращает либо число ячейки, либо ошибку.
    virtual Value Evaluate() const = 0;

    // Возвращает выражение, которое описывает формулу.
    virtual std::string GetExpression() const = 0;
};

// Парсит переданное выражение и возвращает объект формулы.
// Бросает FormulaException в случае, если формула синтаксически некорректна.
std::unique_ptr<FormulaInterface> ParseFormula(std::string expression);