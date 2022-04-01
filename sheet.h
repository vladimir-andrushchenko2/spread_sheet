#pragma once

#include <functional>

#include "cell.h"
#include "common.h"

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const Cell* GetCellPtr(Position pos) const;

    Cell* GetCellPtr(Position pos);

private:
    void AdjustMinPrintableArea(Position pos) {
        cells_.resize(std::max(pos.row + 1, static_cast<int>(cells_.size())));
        cells_[pos.row].resize(std::max(pos.col + 1, static_cast<int>(cells_[pos.row].size())));
    }

    void Print(std::ostream& output, const std::function<void(const CellInterface&)>& PrintCellFunction) const;

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;
};