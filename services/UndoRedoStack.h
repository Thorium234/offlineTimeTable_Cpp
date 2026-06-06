#pragma once
#include <string>
#include <vector>
#include "../timetable/Timetable.h"

struct CellSnapshot {
    int classId;
    int dayIndex;
    int periodIndex;
    TimetableCell before;
    TimetableCell after;
};

class UndoRedoStack {
public:
    explicit UndoRedoStack(size_t maxHistory = 100);

    void push(CellSnapshot cmd);
    bool canUndo() const;
    bool canRedo() const;
    CellSnapshot undo();
    CellSnapshot redo();
    void clear();
    size_t undoCount() const;
    size_t redoCount() const;

    std::string lastActionLabel;
    void setActionLabel(const std::string &label);

private:
    std::vector<CellSnapshot> commands;
    int currentIndex;
    size_t maxSize;
};
