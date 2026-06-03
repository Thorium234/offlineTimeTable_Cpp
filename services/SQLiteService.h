#pragma once

#include <sqlite3.h>
#include <string>
#include <stdexcept>
#include <vector>

class SQLiteService {
public:
    explicit SQLiteService(const std::string &dbPath);
    ~SQLiteService();

    // Execute a SQL statement that does not return rows (e.g., CREATE, INSERT, DELETE)
    void execute(const std::string &sql);

    // Prepare a statement for queries with parameters
    sqlite3_stmt* prepare(const std::string &sql);

    // Helpers for binding parameters
    static void bindInt(sqlite3_stmt *stmt, int index, int value);
    static void bindText(sqlite3_stmt *stmt, int index, const std::string &value);

    // Step through a prepared statement (returns true if a row is available)
    static bool step(sqlite3_stmt *stmt);

    // Finalize a prepared statement
    static void finalize(sqlite3_stmt *stmt);

private:
    sqlite3 *db_ = nullptr;
};
