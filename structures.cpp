#include <algorithm>
#include <cctype>
#include <sstream>

#include "common.h"

const int kLettersInAlphabet = 26;
const int kMaxPositionLength = 17;
const int kMaxLetterCountInPosition = 3;

const Position Position::NONE = {-1, -1};

bool Position::operator==(const Position rhs) const { return row == rhs.row && col == rhs.col; }

bool Position::operator<(const Position rhs) const { return std::tie(row, col) < std::tie(rhs.row, rhs.col); }

bool Position::IsValid() const { return row >= 0 && col >= 0 && row < kMaxRows && col < kMaxColumns; }

std::string Position::ToString() const {
    if (!IsValid()) {
        return "";
    }

    std::string result;
    result.reserve(kMaxPositionLength);
    int c = col;
    while (c >= 0) {
        result.insert(result.begin(), 'A' + c % kLettersInAlphabet);
        c = c / kLettersInAlphabet - 1;
    }

    result += std::to_string(row + 1);

    return result;
}

Position Position::FromString(std::string_view str) {
    auto it =
        std::find_if(str.begin(), str.end(), [](const char c) { return !(std::isalpha(c) && std::isupper(c)); });

    auto letters = str.substr(0, it - str.begin());
    auto digits = str.substr(it - str.begin());

    if (letters.empty() || digits.empty()) {
        return Position::NONE;
    }

    if (letters.size() > kMaxLetterCountInPosition) {
        return Position::NONE;
    }

    if (!std::isdigit(digits[0])) {
        return Position::NONE;
    }

    int row;
    std::istringstream row_in{std::string{digits}};
    if (!(row_in >> row) || !row_in.eof()) {
        return Position::NONE;
    }

    int col = 0;
    for (char ch : letters) {
        col *= kLettersInAlphabet;
        col += ch - 'A' + 1;
    }

    return {row - 1, col - 1};
}

bool Size::operator==(Size rhs) const { return cols == rhs.cols && rows == rhs.rows; }