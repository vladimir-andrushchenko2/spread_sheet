#pragma once

#include <unordered_set>

#include "common.h"
#include "formula.h"

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl;

    class EmptyImpl;

    class TextImpl;

    class FormulaImpl;

private:
    bool WouldIntroduceCircularDependency(const Impl& new_impl) const;

    void InvalidateCacheRecursive(bool force = false);

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::unordered_set<Cell*> parent_nodes_;
    std::unordered_set<Cell*> child_nodes_;
};