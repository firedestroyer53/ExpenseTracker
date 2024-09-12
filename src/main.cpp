#include "main.hpp"

std::unordered_map<string, string> credentials;
std::unordered_map<string, UserDatum> userData;

UserDatum noUser = UserDatum();
UserDatum currentUser = UserDatum();
int latestID = 0;

int main() {
}

bool createAccount(string username, string password) {
    if (credentials[username] != "") { //username is in system
        std::cerr << "Bad create" << std::endl;

        return false;
    }
    if (username == "" || password == "") { // TODO: extend, for now, just empty username/password
        std::cerr << "Bad create" << std::endl;        
        return false;
    }
    credentials[username] = hashPassword(password);
    UserDatum datum;
    datum.username = username;
    userData[username] = datum;
    saveCredential(username, hashPassword(password));
    return true;
}

bool login(string username, string password) {
    if (currentUser.username != noUser.username) return false; // someone is already logged in, bad
    if (credentials[username] == "") { //username is not in system
        std::cerr << "Bad login 1" << std::endl;
        return false;
    }
    if(credentials[username] != hashPassword(password)) { // password incorrect
        std::cerr << "Bad login 2" << std::endl;
        return false;
    }
    currentUser = userData[username];
    return true;
}

void save(UserDatum client) {
    userData[client.username] = client;
    for (auto expense : client.expenses) {
        if(expense.madeThisSession) saveExpense(client.username, expense);
    }
}

void logout() {
    save(currentUser);
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

void saveCredential(string username, string hashedPassword) {
    std::ofstream Creds("files/credentials.txt", std::ios_base::app);
    Creds << username << "," << hashedPassword << "," << "\n";
    Creds.close();
}

void loadCredentials() {
    std::ifstream Creds("files/credentials.txt", std::ios_base::app);
    string line;
    while (getline(Creds, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        string lineStream;
        vector<string> data;
        while(getline(iss, lineStream, ',')) {
            data.push_back(lineStream);
        }
        credentials[data[0]] = data[1];

        UserDatum datum; // setup the user data for each user
        datum.username = data[0];
        userData[data[0]] = datum;
    }
    Creds.close();
}

void createExpense(int amount, string description, string category, int month, int day, int year) {
    if (currentUser.username == noUser.username) return;
    Expense expense = Expense(amount, description, category, month, day, year, latestID - 1, true);
    latestID = latestID - 1;
    currentUser.expenses.push_back(expense);
}

void saveExpense(string username, Expense expense) {
    std::ofstream Data("files/userdata.txt", std::ios_base::app);
    Data << username << "," << expense.amountSpent << "," << expense.description << "," << expense.category << "," << expense.month << "," << expense.day << "," << expense.year << "," << expense.id << "," << "\n";
    Data.close();
}

void loadExpenses() {
    std::ifstream Data("files/userdata.txt");
    string line;
    while (getline(Data, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
    string lineStream;
        vector<string> data;
        while (getline(iss, lineStream, ',')) {
            data.push_back(lineStream);
        }
        Expense expense = Expense(std::stoi(data[1]), data[2], data[3], std::stoi(data[4]), std::stoi(data[5]), std::stoi(data[6]), std::stoi(data[7]), false);
        latestID = std::stoi(data[7]);
        userData[data[0]].expenses.push_back(expense);
    }
    Data.close();
}

void deleteExpense(int expenseID) {
    bool foundExpense = false;
    string username;

    // Find and delete the expense in userData
    for (auto& userDatum : userData) {
        auto& userExpenses = userDatum.second.expenses;
        auto it = std::remove_if(userExpenses.begin(), userExpenses.end(), 
            [expenseID](const Expense& expense) { return expense.id == expenseID; });

        if (it != userExpenses.end()) {
            userExpenses.erase(it, userExpenses.end());
            username = userDatum.first;
            foundExpense = true;
            break;
        }
    }

    if (!foundExpense) {
        std::cerr << "Expense ID " << expenseID << " not found.\n";
        return;
    }

    // Then delete from the file
    std::ifstream Data("files/userdata.txt");
    std::ofstream Temp("files/temp.txt");
    string line;

    while (getline(Data, line)) {
        // Check if the line contains the expense ID
        if (line.find("," + std::to_string(expenseID) + ",") == std::string::npos) {
            Temp << line << "\n";
        }
    }

    Temp.close();
    Data.close();

    // Replace the original file with the temp file
    std::remove("files/userdata.txt");
    std::rename("files/temp.txt", "files/userdata.txt");
}

void setBudget(string category, double budget) {
    currentUser.budgets[category] = budget;
}

double totalSpending(string category) {
    double sum = 0;
    for(auto expense : currentUser.expenses) {
        if(expense.category == category) sum += expense.amountSpent;
    }
    return sum;
}

vector<Expense> getCategory(string category) {
    vector<Expense> fin;
    for (auto expense : currentUser.expenses) {
        if(expense.category == category) fin.push_back(expense);
    }
    return fin;
}