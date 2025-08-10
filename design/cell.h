class Cell : public CellInterface {
public:
    Cell();
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

private:
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual void InvalidateCache() {}  // Добавлено
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl() {
            value_ = text_ = "";
        }

        Value GetValue() const override {
            return value_;
        }

        std::string GetText() const override {
            return text_;
        }
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string_view text) {
            text_ = text;
            if (text[0] == '\'') text = text.substr(1);
            value_ = std::string(text);
        }

        Value GetValue() const override {
            return value_;
        }

        std::string GetText() const override {
            return text_;
        }
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string_view expression) {
            expression = expression.substr(1);
            value_ = std::string(expression);
            formula_ptr_ = ParseFormula(std::string(expression)); 
            text_ = "=" + formula_ptr_->GetExpression();
            cached_value_.reset();  // Инициализация кеша
        }

        Value GetValue() const override {
            if (!cached_value_) {
                cached_value_ = formula_ptr_->Evaluate();
            }
            return *cached_value_;
        }

        std::string GetText() const override {
            return text_;
        }

        void InvalidateCache() override {
            cached_value_.reset();  // Инвалидация кеша
        }

    private:
        std::unique_ptr<FormulaInterface> formula_ptr_;
        mutable std::optional<Value> cached_value_;  
    };

    std::unique_ptr<Impl> impl_;
};