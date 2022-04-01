#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <stack>
#include <string>
#include <unordered_set>

#include "sheet.h"

using namespace std::string_literals;

class Cell::Impl {
public:
    virtual ~Impl() = default;

    virtual Value GetValue() const = 0;

    virtual std::vector<Position> GetReferencedCells() const { return {}; }

    virtual std::string GetText() const = 0;

    virtual void InvalidateCache() {}
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override { return ""; }

    std::string GetText() const override { return ""; }
};

class Cell::TextImpl : public Impl {
public:
    explicit TextImpl(std::string text) : text_(std::move(text)) {
        if (text_.empty()) {
            throw std::logic_error("Should be EmptyImpl instead");
        }
    }

    Value GetValue() const override {
        if (text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() const override { return text_; }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    explicit FormulaImpl(std::string text, const SheetInterface &sheet) : sheet_(sheet) {
        if (text.empty() || text[0] != FORMULA_SIGN) {
            throw std::logic_error("A formula should start with"s + FORMULA_SIGN + "sign"s);
        }

        formula_ = ParseFormula(text.substr(1));
    }

    Value GetValue() const override {
        if (!cache_) {
            cache_ = formula_->Evaluate(sheet_);
        }

        return std::visit([](const auto &x) { return Value(x); }, *cache_);
    }

    std::vector<Position> GetReferencedCells() const override { return formula_->GetReferencedCells(); }

    std::string GetText() const override { return FORMULA_SIGN + formula_->GetExpression(); }

    // bool IsCacheValid() const override { return cache_.has_value(); }

    void InvalidateCache() override { cache_.reset(); }

private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface &sheet_;
    mutable std::optional<FormulaInterface::Value> cache_;
};

// Реализуйте следующие методы
Cell::Cell(Sheet &sheet) : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

Cell::~Cell() {}

std::vector<Position> Cell::GetReferencedCells() const { return impl_->GetReferencedCells(); }

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> new_impl;

    if (text.empty()) {
        new_impl = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        new_impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    } else {
        new_impl = std::make_unique<TextImpl>(std::move(text));
    }

    if (WouldIntroduceCircularDependency(*new_impl)) {
        throw CircularDependencyException("Setting this formula would introduce circular dependency!");
    }

    impl_ = std::move(new_impl);

    for (Cell *outgoing : child_nodes_) {
        outgoing->parent_nodes_.erase(this);
    }

    child_nodes_.clear();

    for (const auto &pos : impl_->GetReferencedCells()) {
        Cell *outgoing = sheet_.GetCellPtr(pos);

        if (!outgoing) {
            sheet_.SetCell(pos, "");
            outgoing = sheet_.GetCellPtr(pos);
        }

        child_nodes_.insert(outgoing);
        outgoing->parent_nodes_.insert(this);
    }

    InvalidateCacheRecursive(true);
}

void Cell::InvalidateCacheRecursive(bool force) {
    impl_->InvalidateCache();

    for (Cell *incoming : parent_nodes_) {
        incoming->InvalidateCacheRecursive();
    }
}

bool Cell::WouldIntroduceCircularDependency(const Impl &new_impl) const {
    if (new_impl.GetReferencedCells().empty()) {
        return false;
    }

    std::unordered_set<const Cell *> referenced;
    for (const auto &pos : new_impl.GetReferencedCells()) {
        referenced.insert(sheet_.GetCellPtr(pos));
    }

    std::unordered_set<const Cell *> visited;
    std::stack<const Cell *> to_visit;
    to_visit.push(this);
    while (!to_visit.empty()) {
        const Cell *current = to_visit.top();
        to_visit.pop();
        visited.insert(current);

        if (referenced.find(current) != referenced.end()) {
            return true;
        }

        for (const Cell *incoming : current->parent_nodes_) {
            if (visited.find(incoming) == visited.end()) {
                to_visit.push(incoming);
            }
        }
    }

    return false;
}

void Cell::Clear() { Set(""); }

Cell::Value Cell::GetValue() const { return impl_->GetValue(); }

std::string Cell::GetText() const { return impl_->GetText(); }