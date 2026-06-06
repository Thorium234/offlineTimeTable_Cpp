"""
Timetable Generator — Web Backend Entry Point

This file is the Flask web server entry point for the timetable generator.
It is launched as a child process by the C++ binary (timetableGen).

To run standalone (e.g., during development):
    python3 main.py

The Flask server provides:
  - REST API for managing teachers, subjects, classes, rooms, lessons
  - Timetable generation (backtracking / greedy)
  - Interactive web UI for viewing and editing the timetable
  - Analytics, conflict detection, and data import/export

Shared database: data/timetableGen.db (SQLite)
"""

from web.app import app, init_db

if __name__ == "__main__":
    init_db()
    app.run(host="0.0.0.0", port=5000, debug=False)
