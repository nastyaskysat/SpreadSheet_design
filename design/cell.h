#pragma once 

#include "common.h" 
#include "formula.h" 
#include <vector>
#include <unordered_set>

class Cell : public CellInterface { 
public: 
    Cell(SheetInterface& sheet);
    ~Cell(); 
 
    void Set(std::string text); 
    void Clear(); 
 
    Value GetValue() const override; 
    std::string GetText() const override; 
    std::vector<Position> GetReferencedCells() const override;
    
    void InvalidateCache();
    void AddDependentCell(Position pos);
    const std::unordered_set<Position>& GetDependentCells() const;
    void ClearDependencies();

private: 
    class Impl { 
    public: 
        virtual ~Impl() = default; 
        virtual Value GetValue() const = 0; 
        virtual std::string GetText() const = 0; 
        virtual std::vector<Position> GetReferencedCells() const = 0;
    }; 
 
    class EmptyImpl : public Impl { 
    public: 
        Value GetValue() const override { 
            return ""; 
        } 
 
        std::string GetText() const override { 
            return ""; 
        } 
        
        std::vector<Position> GetReferencedCells() const override {
            return {};
        }
    }; 
 
    class TextImpl : public Impl { 
    public: 
        TextImpl(std::string text) 
            : text_(std::move(text)) 
        {
            if (!text_.empty() && text_[0] == '\'') {
                value_ = text_.substr(1);
            } else {
                value_ = text_;
            }
        } 
 
        Value GetValue() const override { 
            return value_; 
        } 
 
        std::string GetText() const override { 
            return text_; 
        }
        
        std::vector<Position> GetReferencedCells() const override {
            return {};
        }
        
    private:
        std::string text_;
        std::string value_;
    }; 
 
    class FormulaImpl : public Impl { 
    public: 
        FormulaImpl(std::string expression, const SheetInterface& sheet) 
            : formula_(ParseFormula(expression.substr(1)))
            , sheet_(sheet)
        {} 
 
        Value GetValue() const override { 
            if (!cached_value_) { 
                cached_value_ = formula_->Evaluate(sheet_); 
            } 
            return *cached_value_; 
        } 
 
        std::string GetText() const override { 
            return "=" + formula_->GetExpression(); 
        }
        
        std::vector<Position> GetReferencedCells() const override {
            return formula_->GetReferencedCells();
        }
        
        void InvalidateCache() {
            cached_value_.reset();
        }
 
    private: 
        std::unique_ptr<FormulaInterface> formula_;
        const SheetInterface& sheet_;
        mutable std::optional<Value> cached_value_;   
    }; 
 
    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;
    std::unordered_set<Position> dependent_cells_;
}; 