#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

#include "FormulaAST.h"

using namespace std::literals;

FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const { return category_; }

bool FormulaError::operator==(FormulaError rhs) const { return category_ == rhs.category_; }

std::string_view FormulaError::ToString() const {
    switch (category_) {
        case Category::Ref:
            return "#REF!";
        case Category::Value:
            return "#VALUE!";
        case Category::Div0:
            return "#DIV/0!";
    }
    return "";
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) { return output << fe.ToString(); }

namespace {
class Formula : public FormulaInterface {
public:
    // Реализуйте следующие методы:
    explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression)) {
    } catch (const std::exception& exc) {
        std::throw_with_nested(FormulaException(exc.what()));
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        auto cell_access_func = [&sheet](Position position) -> double {
            if (!position.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }

            const auto* cell = sheet.GetCell(position);

            if (!cell) {
                return 0;
            }

            if (std::holds_alternative<double>(cell->GetValue())) {
                return std::get<double>(cell->GetValue());
            }

            if (std::holds_alternative<std::string>(cell->GetValue())) {
                auto cell_contents = std::get<std::string>(cell->GetValue());

                double value = 0;

                if (!cell_contents.empty()) {
                    std::istringstream in(cell_contents);

                    if (!(in >> value) || !in.eof()) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }

                return value;
            }

            throw FormulaError(std::get<FormulaError>(cell->GetValue()));
        };

        try {
            return ast_.Execute(cell_access_func);
        } catch (const FormulaError& error) {
            return error;
        }
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells;
        for (auto cell : ast_.GetCells()) {
            if (cell.IsValid()) {
                cells.push_back(cell);
            }
        }

        cells.resize(std::unique(cells.begin(), cells.end()) - cells.begin());
        return cells;
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

private:
    FormulaAST ast_;
};

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}