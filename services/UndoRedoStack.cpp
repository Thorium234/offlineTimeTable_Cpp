#include "UndoRedoStack.h"

UndoRedoStack::UndoRedoStack(size_t maxHistory)
    : currentIndex(-1), maxSize(maxHistory) {}

void UndoRedoStack::push(CellSnapshot cmd) {
    // Remove any redo history beyond current position
    if (currentIndex + 1 < static_cast<int>(commands.size())) {
        commands.resize(static_cast<size_t>(currentIndex + 1));
    }

    commands.push_back(cmd);

    // Enforce max size
    if (commands.size() > maxSize) {
        commands.erase(commands.begin(), commands.begin() + static_cast<long>(commands.size() - maxSize));
    }

    currentIndex = static_cast<int>(commands.size()) - 1;
}

bool UndoRedoStack::canUndo() const {
    return currentIndex >= 0;
}

bool UndoRedoStack::canRedo() const {
    return currentIndex + 1 < static_cast<int>(commands.size());
}

CellSnapshot UndoRedoStack::undo() {
    if (!canUndo()) return {};
    const auto &cmd = commands[currentIndex];
    CellSnapshot result = cmd;
    --currentIndex;
    return result;
}

CellSnapshot UndoRedoStack::redo() {
    if (!canRedo()) return {};
    ++currentIndex;
    return commands[currentIndex];
}

void UndoRedoStack::clear() {
    commands.clear();
    currentIndex = -1;
    lastActionLabel.clear();
}

size_t UndoRedoStack::undoCount() const {
    return static_cast<size_t>(currentIndex + 1);
}

size_t UndoRedoStack::redoCount() const {
    if (commands.empty()) return 0;
    return commands.size() - static_cast<size_t>(currentIndex + 1);
}

void UndoRedoStack::setActionLabel(const std::string &label) {
    lastActionLabel = label;
}
