#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

#include "cell.h"
#include "common.h"

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid pos");
    }

    AdjustMinPrintableArea(pos);

    auto &cell = cells_[pos.row][pos.col];
    if (!cell) {
        cell = std::make_unique<Cell>(*this);
    }
    cell->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const { return GetCellPtr(pos); }

CellInterface* Sheet::GetCell(Position pos) { return GetCellPtr(pos); }

const Cell* Sheet::GetCellPtr(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid pos");
    }

    if (pos.row >= static_cast<int>(cells_.size()) || pos.col >= static_cast<int>(cells_[pos.row].size())) {
        return nullptr;
    }

    return cells_[pos.row][pos.col].get();
}

Cell* Sheet::GetCellPtr(Position pos) {
    return const_cast<Cell*>(static_cast<const Sheet&>(*this).GetCellPtr(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid pos");
    }

    if (pos.row < static_cast<int>(cells_.size()) && pos.col < static_cast<int>(cells_[pos.row].size())) {
        if (auto &cell = cells_[pos.row][pos.col]) {
            cell->Clear();
            cell.reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size size;
    for (int row = 0; row < static_cast<int>(cells_.size()); ++row) {
        for (int col = static_cast<int>(cells_[row].size()) - 1; col >= 0; --col) {
            if (const auto &cell = cells_[row][col]) {
                if (!cell->GetText().empty()) {
                    size.rows = std::max(size.rows, row + 1);
                    size.cols = std::max(size.cols, col + 1);
                    break;
                }
            }
        }
    }
    return size;
}

void Sheet::Print(std::ostream& output,
                  const std::function<void(const CellInterface&)>& PrintCellFunction) const {
    auto size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        int last_non_empty_cell = static_cast<int>(cells_[row].size());

        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }

            if (col < last_non_empty_cell) {
                if (const auto &cell = cells_[row][col]) {
                    PrintCellFunction(*cell);
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    Print(output, [&](const CellInterface& cell) {
        std::visit([&](const auto& value) { output << value; }, cell.GetValue());
    });
}

void Sheet::PrintTexts(std::ostream& output) const {
    Print(output, [&output](const CellInterface& cell) { output << cell.GetText(); });
}

std::unique_ptr<SheetInterface> CreateSheet() { return std::make_unique<Sheet>(); }