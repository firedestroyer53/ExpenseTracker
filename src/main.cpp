#include "main.hpp"

sqlite3* db;

std::unordered_map<string, string> credentials;
std::unordered_map<string, UserDatum> userData;

UserDatum noUser = UserDatum();
UserDatum currentUser = UserDatum();
int latestID = 0;
/*
void main(int argc, char **argv) {
    QApplication app(argc, argv);
    auto myWindow = new QWidget();
    auto myButton = new QPushButton(myWindow);
    myWindow->show();
    return app.exec();
}
GUI testing
*/
int main() {
    if (sqlite3_open("files/expenses.db", &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    initializeDatabase();
    loadCredentials();
    loadExpenses();
    login("Gus", "1");

    logout();

    sqlite3_close(db);
    return 0;
}

int executeQuery(const char* sql, int (*callback)(void*, int, char**, char**), void* data) {
    char* errmsg;
    int rc = sqlite3_exec(db, sql, callback, data, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }
    return rc;
}

int callback(void* data, int argc, char** argv, char** azColName) {
    return 0;
}

void initializeDatabase() {
    const char* createUsersTableSQL = 
        "CREATE TABLE IF NOT EXISTS users ("
        "username TEXT PRIMARY KEY, "
        "password TEXT);";
    executeQuery(createUsersTableSQL, callback, 0);

    const char* createExpensesTableSQL = 
        "CREATE TABLE IF NOT EXISTS expenses ("
        "username TEXT, "
        "amount REAL, "
        "description TEXT, "
        "category TEXT, "
        "month INTEGER, "
        "day INTEGER, "
        "year INTEGER, "
        "id INTEGER PRIMARY KEY, "
        "madeThisSession INTEGER);";
    executeQuery(createExpensesTableSQL, callback, 0);
}

void loadCredentials() {
    const char* selectCredentialsSQL = "SELECT username, password FROM users;";
    auto callback = [](void* data, int argc, char** argv, char** azColName) -> int {
        std::unordered_map<string, string>* credentials = static_cast<std::unordered_map<string, string>*>(data);
        if (argc == 2) {
            // Ensure userData entry is created
            UserDatum datum;
            datum.username = argv[0];
            (*credentials)[argv[0]] = argv[1]; // This is redundant but keeps it clear
            userData[argv[0]] = datum;
        }
        return 0;
    };

    executeQuery(selectCredentialsSQL, callback, &credentials);
}

void loadExpenses() {
    const char* selectExpensesSQL = "SELECT username, amount, description, category, month, day, year, id, madeThisSession FROM expenses;";
    auto callback = [](void* data, int argc, char** argv, char** azColName) -> int {
        std::unordered_map<string, UserDatum>* userData = static_cast<std::unordered_map<string, UserDatum>*>(data);
        if (argc == 9) {
            Expense expense(std::stod(argv[1]), argv[2], argv[3], 
                            std::stoi(argv[4]), std::stoi(argv[5]),
                            std::stoi(argv[6]), std::stoi(argv[7]),
                             std::stoi(argv[8]));
            (*userData)[argv[0]].expenses.push_back(expense);
        }
        return 0;
    };

    executeQuery(selectExpensesSQL, callback, &userData);
}

bool createAccount(string username, string password) {
    if (credentials.find(username) != credentials.end()) { // username is in system
        std::cerr << "Bad create" << std::endl;
        return false;
    }
    if (username.empty() || password.empty()) { // TODO: extend, for now, just empty username/password
        std::cerr << "Bad create" << std::endl;        
        return false;
    }

    string hashedPassword = hashPassword(password);

    std::string insertSQL = "INSERT INTO users (username, password) VALUES ('"
                            + username + "', '" + hashedPassword + "');";
    if (executeQuery(insertSQL.c_str(), nullptr, nullptr) != SQLITE_OK) {
        return false;
    }

    UserDatum datum;
    datum.username = username;
    userData[username] = datum;
    credentials[username] = hashedPassword;

    return true;
}

bool login(string username, string password) {
    if (currentUser.username != noUser.username) { // someone is already logged in, prompt user to log out probably?
        std::cerr << "Bad login 0" << std::endl;
        if(false) {  // for now just deny the login, put a condition here or something
            logout();
        }
        else return false; 
    }
    if (credentials.find(username) == credentials.end()) { // username is not in system
        std::cerr << "Bad login 1" << std::endl;
        return false;
    }
    if (credentials[username] != hashPassword(password)) { // password incorrect
        std::cerr << "Bad login 2" << std::endl;
        return false;
    }
    currentUser = userData[username];
    return true;
}

void save() {
    for (auto& expense : currentUser.expenses) {
        if (expense.madeThisSession) {
            saveExpense(currentUser.username, expense);
        }
    }
}

void logout() {
    save();
    currentUser = noUser;
}

string hashPassword(string password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password.c_str(), password.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << (int)hash[i];
    }
    return ss.str();
}

void saveExpense(string username, Expense expense) {
    std::string insertSQL = "INSERT INTO expenses (username, amount, description, category, month, day, year, id, madeThisSession) VALUES ("
        "'" + username + "', "
        + std::to_string(expense.amountSpent) + ", "
        "'" + expense.description + "', "
        "'" + expense.category + "', "
        + std::to_string(expense.month) + ", "
        + std::to_string(expense.day) + ", "
        + std::to_string(expense.year) + ", "
        + std::to_string(expense.id) + ", "
        + std::to_string(expense.madeThisSession) + ");";
    executeQuery(insertSQL.c_str(), nullptr, nullptr);
}

void createExpense(double amount, string description, string category, int month, int day, int year) {
    if (currentUser.username == noUser.username) return; // no user is logged in

    Expense expense(amount, description, category, month, day, year, latestID--, true);
    currentUser.expenses.push_back(expense);
}

void deleteExpense(int expenseID) {
    std::string deleteSQL = "DELETE FROM expenses WHERE id = " + std::to_string(expenseID) + ";";
    if (executeQuery(deleteSQL.c_str(), nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to delete expense with ID " << expenseID << " from database." << std::endl;
    }

    for (auto& userDatum : userData) {
        auto& userExpenses = userDatum.second.expenses;
        userExpenses.erase(std::remove_if(userExpenses.begin(), userExpenses.end(),
            [expenseID](const Expense& expense) { return expense.id == expenseID; }),
            userExpenses.end());
    }
    currentUser = userData[currentUser.username]; // update currentUser with changes
}

void setBudget(string category, double budget) {
    currentUser.budgets[category] = budget;
}

double totalSpending(string category) {
    double sum = 0;
    for (auto& expense : currentUser.expenses) {
        if (expense.category == category) sum += expense.amountSpent;
    }
    return sum;
}

bool deleteAccount(string username) {
    // Check if the user exists
    if (credentials.find(username) == credentials.end()) {
        std::cerr << "User does not exist." << std::endl;
        return false;
    }

    // Remove the user's expenses
    std::string deleteExpensesSQL = "DELETE FROM expenses WHERE username = '" + username + "';";
    if (executeQuery(deleteExpensesSQL.c_str(), nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to delete user expenses." << std::endl;
        return false;
    }

    // Remove the user credentials
    std::string deleteUserSQL = "DELETE FROM users WHERE username = '" + username + "';";
    if (executeQuery(deleteUserSQL.c_str(), nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to delete user credentials." << std::endl;
        return false;
    }

    // Update in-memory data structures
    credentials.erase(username);
    userData.erase(username);

    // If the current user is the one being deleted, log out
    if (currentUser.username == username) {
        logout();
    }

    return true;
}
