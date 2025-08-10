#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <vector>

class CellHasher {
public:
    size_t operator()(const Position p) const {
        return std::hash<std::string>()(p.ToString());
    }
};

class CellComparator {
public:
    bool operator()(const Position& lhs, const Position& rhs) const {
        return lhs == rhs;
    }
};

class Sheet : public SheetInterface {
public:
    using Table = std::unordered_map<Position, std::unique_ptr<Cell>, CellHasher, CellComparator>;

    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override {
        if (!pos.IsValid()) {
            throw InvalidPositionException("Invalid cell position");
        }

        auto& cell = GetOrCreateCell(pos);
        cell->Set(std::move(text));

        // Обновляем зависимости
        UpdateDependencies(pos, GetReferencedCells(*cell));
    }

    const CellInterface* GetCell(Position pos) const override {
        return const_cast<Sheet*>(this)->GetCell(pos);
    }

    CellInterface* GetCell(Position pos) override {
        if (!pos.IsValid()) {
            throw InvalidPositionException("Invalid cell position");
        }

        auto it = cells_.find(pos);
        if (it == cells_.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    void ClearCell(Position pos) override {
        if (!pos.IsValid()) {
            throw InvalidPositionException("Invalid cell position");
        }

        auto it = cells_.find(pos);
        if (it != cells_.end()) {
            // Удаляем зависимости перед очисткой
            RemoveDependencies(pos);
            cells_.erase(it);
        }
    }

    Size GetPrintableSize() const override {
        if (cells_.empty()) {
            return {0, 0};
        }

        int max_row = 0;
        int max_col = 0;

        for (const auto& [pos, _] : cells_) {
            max_row = std::max(max_row, pos.row + 1);
            max_col = std::max(max_col, pos.col + 1);
        }

        return {max_row, max_col};
    }

    void PrintValues(std::ostream& output) const override {
        auto size = GetPrintableSize();
        for (int row = 0; row < size.rows; ++row) {
            for (int col = 0; col < size.cols; ++col) {
                if (col > 0) {
                    output << '\t';
                }

                Position pos{row, col};
                if (cells_.count(pos)) {
                    const auto& cell = cells_.at(pos);
                    auto value = cell->GetValue();
                    if (std::holds_alternative<double>(value)) {
                        output << std::get<double>(value);
                    } else if (std::holds_alternative<std::string>(value)) {
                        output << std::get<std::string>(value);
                    } else {
                        output << std::get<FormulaError>(value);
                    }
                }
            }
            output << '\n';
        }
    }

    void PrintTexts(std::ostream& output) const override {
        auto size = GetPrintableSize();
        for (int row = 0; row < size.rows; ++row) {
            for (int col = 0; col < size.cols; ++col) {
                if (col > 0) {
                    output << '\t';
                }

                Position pos{row, col};
                if (cells_.count(pos)) {
                    output << cells_.at(pos)->GetText();
                }
            }
            output << '\n';
        }
    }

private:
    Table cells_;
    std::unordered_map<Position, std::vector<Position>, CellHasher, CellComparator> dependent_cells_;
    std::unordered_map<Position, std::vector<Position>, CellHasher, CellComparator> reverse_dependencies_;

    Cell* GetOrCreateCell(Position pos) {
        auto& cell_ptr = cells_[pos];
        if (!cell_ptr) {
            cell_ptr = std::make_unique<Cell>();
        }
        return cell_ptr.get();
    }

    void UpdateDependencies(Position pos, const std::vector<Position>& referenced_cells) {
        RemoveDependencies(pos);

        for (const auto& ref_pos : referenced_cells) {
            dependent_cells_[ref_pos].push_back(pos);
            reverse_dependencies_[pos].push_back(ref_pos);
        }

        InvalidateCellWithDependents(pos);
    }

    void RemoveDependencies(Position pos) {
        if (reverse_dependencies_.count(pos)) {
            for (const auto& ref_pos : reverse_dependencies_.at(pos)) {
                auto& dependents = dependent_cells_[ref_pos];
                dependents.erase(std::remove(dependents.begin(), dependents.end(), pos), dependents.end());
            }
            reverse_dependencies_.erase(pos);
        }

        if (dependent_cells_.count(pos)) {
            for (const auto& dependent_pos : dependent_cells_.at(pos)) {
                if (reverse_dependencies_.count(dependent_pos)) {
                    auto& refs = reverse_dependencies_.at(dependent_pos);
                    refs.erase(std::remove(refs.begin(), refs.end(), pos), refs.end());
                }
            }
            dependent_cells_.erase(pos);
        }
    }

    void InvalidateCellWithDependents(Position pos) {
        if (cells_.count(pos)) {
            cells_.at(pos)->InvalidateCache();
        }

        if (dependent_cells_.count(pos)) {
            for (const auto& dependent_pos : dependent_cells_.at(pos)) {
                InvalidateCellWithDependents(dependent_pos);
            }
        }
    }

    std::vector<Position> GetReferencedCells(const Cell& cell) {
        auto value = cell.GetValue();
        if (std::holds_alternative<FormulaError>(value)) {
            return {};
        }
        return cell.GetReferencedCells();
    }
};