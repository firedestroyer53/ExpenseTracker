#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sqlite3.h>
#include <openssl/sha.h>
#include <sstream>
#include <algorithm>
#include <cassert>

using std::vector;
using std::string;

struct Expense {
    double amountSpent;
    string description;
    int month;
    int day; 
    int year;
    int id;
    bool madeThisSession;
    string category;

    Expense() {

    }
    Expense(double amountSpent, string description, string category, int month, int day, int year, int id, bool madeThisSession) {
        this->amountSpent = amountSpent;
        this->description = description;
        this->month = month;
        this->day = day;
        this->year = year;
        this->id = id;
        this->madeThisSession = madeThisSession;
        this->category = category;
    }
};


struct UserDatum {
    string username;
    vector<Expense> expenses;
    std::unordered_map<string, double> budgets;
};

bool login(string, string);
bool createAccount(string, string);
void logout();
void save(UserDatum);
string hashPassword(string);
void saveExpense(string, Expense);
void loadCredentials();
void loadExpenses();
void createExpense(double, string, string, int, int, int);
void deleteExpense(int);
void setBudget(string, double);
double totalSpending(string);
vector<Expense> filterCategory(string);
vector<Expense> sortByDate(vector<Expense>);
bool deleteAccount(string);

// SQLite Database Functions
void initializeDatabase();
int executeQuery(const char* sql, int (*callback)(void*, int, char**, char**), void* data);
int callback(void* data, int argc, char** argv, char** azColName);
