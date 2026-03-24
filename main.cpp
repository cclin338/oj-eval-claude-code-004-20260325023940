#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

// Constants
const string ROOT_USERNAME = "root";
const string ROOT_PASSWORD = "sjtu";
const int ROOT_PRIVILEGE = 7;

// File names
const string USER_FILE = "users.dat";
const string BOOK_FILE = "books.dat";
const string FINANCE_FILE = "finance.dat";
const string SELECTED_BOOK_FILE = "selected.dat";
const string LOGIN_STACK_FILE = "login_stack.dat";

// Data structures
struct User {
    char userID[31];        // 30 chars + null terminator
    char password[31];      // 30 chars + null terminator
    char username[31];      // 30 chars + null terminator
    int privilege;

    User() {
        memset(userID, 0, sizeof(userID));
        memset(password, 0, sizeof(password));
        memset(username, 0, sizeof(username));
        privilege = 0;
    }

    User(const string& id, const string& pass, const string& name, int priv) {
        strncpy(userID, id.c_str(), 30);
        userID[30] = '\0';
        strncpy(password, pass.c_str(), 30);
        password[30] = '\0';
        strncpy(username, name.c_str(), 30);
        username[30] = '\0';
        privilege = priv;
    }

    string getUserID() const { return string(userID); }
    string getPassword() const { return string(password); }
    string getUsername() const { return string(username); }
};

struct Book {
    char ISBN[21];          // 20 chars + null terminator
    char bookName[61];      // 60 chars + null terminator
    char author[61];        // 60 chars + null terminator
    char keyword[61];       // 60 chars + null terminator
    double price;
    int quantity;

    Book() {
        memset(ISBN, 0, sizeof(ISBN));
        memset(bookName, 0, sizeof(bookName));
        memset(author, 0, sizeof(author));
        memset(keyword, 0, sizeof(keyword));
        price = 0.0;
        quantity = 0;
    }

    Book(const string& isbn) {
        memset(ISBN, 0, sizeof(ISBN));
        memset(bookName, 0, sizeof(bookName));
        memset(author, 0, sizeof(author));
        memset(keyword, 0, sizeof(keyword));
        strncpy(ISBN, isbn.c_str(), 20);
        ISBN[20] = '\0';
        price = 0.0;
        quantity = 0;
    }

    string getISBN() const { return string(ISBN); }
    string getBookName() const { return string(bookName); }
    string getAuthor() const { return string(author); }
    string getKeyword() const { return string(keyword); }
};

struct FinanceRecord {
    double income;
    double expenditure;

    FinanceRecord() : income(0.0), expenditure(0.0) {}
    FinanceRecord(double inc, double exp) : income(inc), expenditure(exp) {}
};

// Global state (indexes and current state only - main data in files)
vector<User> users;  // Loaded at startup, updated on changes
vector<Book> books;  // Loaded at startup, updated on changes
vector<string> loginStack;  // userIDs of logged in users
string selectedBookISBN;  // ISBN of selected book
map<string, int> userIndex; // userID -> index in users vector
map<string, int> bookIndex; // ISBN -> index in books vector
vector<FinanceRecord> financeRecords;  // Loaded at startup, updated on changes

// Helper functions
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

bool isValidUserID(const string& id) {
    if (id.empty() || id.length() > 30) return false;
    for (char c : id) {
        if (!isalnum(c) && c != '_') return false;
    }
    return true;
}

bool isValidPassword(const string& pass) {
    if (pass.empty() || pass.length() > 30) return false;
    for (char c : pass) {
        if (!isalnum(c) && c != '_') return false;
    }
    return true;
}

bool isValidUsername(const string& name) {
    if (name.empty() || name.length() > 30) return false;
    for (char c : name) {
        if (c < 32 || c > 126) return false;
    }
    return true;
}

bool isValidISBN(const string& isbn) {
    if (isbn.empty() || isbn.length() > 20) return false;
    for (char c : isbn) {
        if (c < 32 || c > 126) return false;
    }
    return true;
}

bool isValidBookName(const string& name) {
    if (name.empty() || name.length() > 60) return false;
    for (char c : name) {
        if (c < 32 || c > 126 || c == '\"') return false;
    }
    return true;
}

bool isValidAuthor(const string& author) {
    if (author.empty() || author.length() > 60) return false;
    for (char c : author) {
        if (c < 32 || c > 126 || c == '\"') return false;
    }
    return true;
}

bool isValidKeyword(const string& keyword) {
    if (keyword.empty() || keyword.length() > 60) return false;
    for (char c : keyword) {
        if (c < 32 || c > 126 || c == '\"') return false;
    }
    return true;
}

bool isValidPrivilege(const string& privStr) {
    if (privStr.length() != 1) return false;
    char c = privStr[0];
    return c == '1' || c == '3' || c == '7';
}

int getPrivilegeFromStr(const string& privStr) {
    return privStr[0] - '0';
}

bool isValidQuantity(const string& qtyStr) {
    if (qtyStr.empty() || qtyStr.length() > 10) return false;
    for (char c : qtyStr) {
        if (!isdigit(c)) return false;
    }
    try {
        long long qty = stoll(qtyStr);
        return qty > 0 && qty <= 2147483647;
    } catch (...) {
        return false;
    }
}

bool isValidPrice(const string& priceStr) {
    if (priceStr.empty() || priceStr.length() > 13) return false;
    int dotCount = 0;
    for (char c : priceStr) {
        if (c == '.') dotCount++;
        else if (!isdigit(c)) return false;
    }
    if (dotCount > 1) return false;

    size_t dotPos = priceStr.find('.');
    if (dotPos != string::npos && priceStr.length() - dotPos - 1 > 2) {
        return false;
    }

    try {
        double price = stod(priceStr);
        return price >= 0;
    } catch (...) {
        return false;
    }
}

int getCurrentPrivilege() {
    if (loginStack.empty()) return 0;
    string currentUserID = loginStack.back();
    auto it = userIndex.find(currentUserID);
    if (it == userIndex.end()) return 0;
    return users[it->second].privilege;
}

// File operations
void initializeFiles() {
    // Create empty files if they don't exist
    ofstream userFile(USER_FILE, ios::binary | ios::app);
    ofstream bookFile(BOOK_FILE, ios::binary | ios::app);
    ofstream financeFile(FINANCE_FILE, ios::binary | ios::app);
    ofstream selectedFile(SELECTED_BOOK_FILE, ios::app);
    ofstream loginFile(LOGIN_STACK_FILE, ios::app);

    userFile.close();
    bookFile.close();
    financeFile.close();
    selectedFile.close();
    loginFile.close();
}

void loadUsers() {
    users.clear();
    userIndex.clear();

    ifstream file(USER_FILE, ios::binary);
    if (!file) return;

    User user;
    while (file.read(reinterpret_cast<char*>(&user), sizeof(User))) {
        users.push_back(user);
        userIndex[user.getUserID()] = users.size() - 1;
    }
    file.close();

    // Check if root exists
    bool rootExists = false;
    for (const auto& u : users) {
        if (u.getUserID() == ROOT_USERNAME) {
            rootExists = true;
            break;
        }
    }

    if (!rootExists) {
        User root(ROOT_USERNAME, ROOT_PASSWORD, "superadmin", ROOT_PRIVILEGE);
        users.push_back(root);
        userIndex[ROOT_USERNAME] = users.size() - 1;

        // Save to file
        ofstream outFile(USER_FILE, ios::binary | ios::app);
        if (outFile) {
            outFile.write(reinterpret_cast<const char*>(&root), sizeof(User));
            outFile.close();
        }
    }
}

void saveUsers() {
    ofstream file(USER_FILE, ios::binary);
    if (!file) return;

    for (const auto& user : users) {
        file.write(reinterpret_cast<const char*>(&user), sizeof(User));
    }
    file.close();
}

void loadBooks() {
    books.clear();
    bookIndex.clear();

    ifstream file(BOOK_FILE, ios::binary);
    if (!file) return;

    Book book;
    while (file.read(reinterpret_cast<char*>(&book), sizeof(Book))) {
        books.push_back(book);
        bookIndex[book.getISBN()] = books.size() - 1;
    }
    file.close();
}

void saveBooks() {
    ofstream file(BOOK_FILE, ios::binary);
    if (!file) return;

    for (const auto& book : books) {
        file.write(reinterpret_cast<const char*>(&book), sizeof(Book));
    }
    file.close();
}

void loadFinance() {
    financeRecords.clear();

    ifstream file(FINANCE_FILE, ios::binary);
    if (!file) return;

    FinanceRecord record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(FinanceRecord))) {
        financeRecords.push_back(record);
    }
    file.close();
}

void saveFinance() {
    ofstream file(FINANCE_FILE, ios::binary);
    if (!file) return;

    for (const auto& record : financeRecords) {
        file.write(reinterpret_cast<const char*>(&record), sizeof(FinanceRecord));
    }
    file.close();
}

void loadLoginStack() {
    loginStack.clear();

    ifstream file(LOGIN_STACK_FILE);
    if (!file) return;

    string line;
    while (getline(file, line)) {
        string userID = trim(line);
        if (!userID.empty()) {
            loginStack.push_back(userID);
        }
    }
    file.close();
}

void saveLoginStack() {
    ofstream file(LOGIN_STACK_FILE);
    if (!file) return;

    for (const auto& userID : loginStack) {
        file << userID << "\n";
    }
    file.close();
}

void loadSelectedBook() {
    selectedBookISBN.clear();

    ifstream file(SELECTED_BOOK_FILE);
    if (!file) return;

    string line;
    getline(file, line);
    selectedBookISBN = trim(line);
    file.close();
}

void saveSelectedBook() {
    ofstream file(SELECTED_BOOK_FILE);
    if (!file) return;

    file << selectedBookISBN << "\n";
    file.close();
}

void saveAll() {
    saveUsers();
    saveBooks();
    saveFinance();
    saveLoginStack();
    saveSelectedBook();
}

// Command implementations
bool suCommand(const vector<string>& args) {
    if (args.size() < 2 || args.size() > 3) return false;

    string userID = args[1];
    string password = "";

    if (args.size() == 3) {
        password = args[2];
    }

    auto it = userIndex.find(userID);
    if (it == userIndex.end()) {
        return false;
    }

    User& user = users[it->second];

    if (password.empty()) {
        // Password can be omitted only if current privilege > target privilege
        if (loginStack.empty() || getCurrentPrivilege() <= user.privilege) {
            return false;
        }
    } else {
        if (user.getPassword() != password) {
            return false;
        }
    }

    loginStack.push_back(userID);
    selectedBookISBN = "";
    saveLoginStack();
    saveSelectedBook();
    return true;
}

bool logoutCommand(const vector<string>& args) {
    if (args.size() != 1) return false;

    if (loginStack.empty()) {
        return false;
    }

    loginStack.pop_back();
    selectedBookISBN = "";
    saveLoginStack();
    saveSelectedBook();
    return true;
}

bool registerCommand(const vector<string>& args) {
    if (args.size() != 4) return false;

    string userID = args[1];
    string password = args[2];
    string username = args[3];

    if (!isValidUserID(userID) || !isValidPassword(password) || !isValidUsername(username)) {
        return false;
    }

    if (userIndex.find(userID) != userIndex.end()) {
        return false;
    }

    User newUser(userID, password, username, 1);
    users.push_back(newUser);
    userIndex[userID] = users.size() - 1;

    // Save to file
    ofstream file(USER_FILE, ios::binary | ios::app);
    if (file) {
        file.write(reinterpret_cast<const char*>(&newUser), sizeof(User));
        file.close();
    }

    return true;
}

bool passwdCommand(const vector<string>& args) {
    if (args.size() < 3 || args.size() > 4) return false;

    string userID = args[1];
    string currentPassword = "";
    string newPassword = "";

    if (args.size() == 3) {
        newPassword = args[2];

        if (loginStack.empty() || getCurrentPrivilege() != ROOT_PRIVILEGE) {
            return false;
        }
    } else {
        currentPassword = args[2];
        newPassword = args[3];
    }

    auto it = userIndex.find(userID);
    if (it == userIndex.end()) {
        return false;
    }

    User& user = users[it->second];

    if (!currentPassword.empty() && user.getPassword() != currentPassword) {
        return false;
    }

    if (!isValidPassword(newPassword)) {
        return false;
    }

    // Update password
    strncpy(user.password, newPassword.c_str(), 30);
    user.password[30] = '\0';

    saveUsers();
    return true;
}

bool useraddCommand(const vector<string>& args) {
    if (args.size() != 5) return false;

    if (loginStack.empty() || getCurrentPrivilege() < 3) {
        return false;
    }

    string userID = args[1];
    string password = args[2];
    string privilegeStr = args[3];
    string username = args[4];

    if (!isValidUserID(userID) || !isValidPassword(password) ||
        !isValidPrivilege(privilegeStr) || !isValidUsername(username)) {
        return false;
    }

    int privilege = getPrivilegeFromStr(privilegeStr);

    if (privilege >= getCurrentPrivilege()) {
        return false;
    }

    if (userIndex.find(userID) != userIndex.end()) {
        return false;
    }

    User newUser(userID, password, username, privilege);
    users.push_back(newUser);
    userIndex[userID] = users.size() - 1;

    // Save to file
    ofstream file(USER_FILE, ios::binary | ios::app);
    if (file) {
        file.write(reinterpret_cast<const char*>(&newUser), sizeof(User));
        file.close();
    }

    return true;
}

bool deleteCommand(const vector<string>& args) {
    if (args.size() != 2) return false;

    if (loginStack.empty() || getCurrentPrivilege() != ROOT_PRIVILEGE) {
        return false;
    }

    string userID = args[1];

    auto it = userIndex.find(userID);
    if (it == userIndex.end()) {
        return false;
    }

    for (const auto& loggedInUser : loginStack) {
        if (loggedInUser == userID) {
            return false;
        }
    }

    int index = it->second;
    users.erase(users.begin() + index);
    userIndex.erase(it);

    for (auto& pair : userIndex) {
        if (pair.second > index) {
            pair.second--;
        }
    }

    saveUsers();
    return true;
}

bool showCommand(const vector<string>& args) {
    if (loginStack.empty() || getCurrentPrivilege() < 1) {
        return false;
    }

    vector<Book> results;

    if (args.size() == 1) {
        results = books;
    } else if (args.size() == 2) {
        string param = args[1];

        if (param.find("-ISBN=") == 0) {
            string isbn = param.substr(6);
            if (!isValidISBN(isbn) || isbn.empty()) return false;

            auto it = bookIndex.find(isbn);
            if (it != bookIndex.end()) {
                results.push_back(books[it->second]);
            }
        } else if (param.find("-name=\"") == 0 && param.back() == '\"') {
            string name = param.substr(7, param.length() - 8);
            if (!isValidBookName(name) || name.empty()) return false;

            for (const auto& book : books) {
                if (book.getBookName() == name) {
                    results.push_back(book);
                }
            }
        } else if (param.find("-author=\"") == 0 && param.back() == '\"') {
            string author = param.substr(9, param.length() - 10);
            if (!isValidAuthor(author) || author.empty()) return false;

            for (const auto& book : books) {
                if (book.getAuthor() == author) {
                    results.push_back(book);
                }
            }
        } else if (param.find("-keyword=\"") == 0 && param.back() == '\"') {
            string keyword = param.substr(10, param.length() - 11);
            if (!isValidKeyword(keyword) || keyword.empty()) return false;

            if (keyword.find('|') != string::npos) {
                return false;
            }

            for (const auto& book : books) {
                string bookKeywords = book.getKeyword();
                vector<string> keywords = split(bookKeywords, '|');
                for (const auto& kw : keywords) {
                    if (kw == keyword) {
                        results.push_back(book);
                        break;
                    }
                }
            }
        } else {
            return false;
        }
    } else {
        return false;
    }

    sort(results.begin(), results.end(), [](const Book& a, const Book& b) {
        return a.getISBN() < b.getISBN();
    });

    for (const auto& book : results) {
        cout << book.getISBN() << "\t" << book.getBookName() << "\t" << book.getAuthor()
             << "\t" << book.getKeyword() << "\t" << fixed << setprecision(2)
             << book.price << "\t" << book.quantity << "\n";
    }

    if (results.empty()) {
        cout << "\n";
    }

    return true;
}

bool buyCommand(const vector<string>& args) {
    if (args.size() != 3) return false;

    if (loginStack.empty() || getCurrentPrivilege() < 1) {
        return false;
    }

    string isbn = args[1];
    string qtyStr = args[2];

    if (!isValidISBN(isbn) || !isValidQuantity(qtyStr)) {
        return false;
    }

    int quantity = stoi(qtyStr);

    auto it = bookIndex.find(isbn);
    if (it == bookIndex.end()) {
        return false;
    }

    Book& book = books[it->second];

    if (book.quantity < quantity) {
        return false;
    }

    double totalCost = book.price * quantity;
    book.quantity -= quantity;

    financeRecords.push_back(FinanceRecord(totalCost, 0.0));

    saveBooks();
    saveFinance();

    cout << fixed << setprecision(2) << totalCost << "\n";
    return true;
}

bool selectCommand(const vector<string>& args) {
    if (args.size() != 2) return false;

    if (loginStack.empty() || getCurrentPrivilege() < 3) {
        return false;
    }

    string isbn = args[1];

    if (!isValidISBN(isbn)) {
        return false;
    }

    auto it = bookIndex.find(isbn);
    if (it == bookIndex.end()) {
        Book newBook(isbn);
        books.push_back(newBook);
        bookIndex[isbn] = books.size() - 1;
        selectedBookISBN = isbn;

        // Save new book to file
        ofstream file(BOOK_FILE, ios::binary | ios::app);
        if (file) {
            file.write(reinterpret_cast<const char*>(&newBook), sizeof(Book));
            file.close();
        }
    } else {
        selectedBookISBN = isbn;
    }

    saveSelectedBook();
    return true;
}

bool modifyCommand(const vector<string>& args) {
    if (args.size() < 2) return false;

    if (loginStack.empty() || getCurrentPrivilege() < 3) {
        return false;
    }

    if (selectedBookISBN.empty()) {
        return false;
    }

    auto it = bookIndex.find(selectedBookISBN);
    if (it == bookIndex.end()) {
        return false;
    }

    Book& book = books[it->second];
    map<string, bool> paramUsed;

    for (size_t i = 1; i < args.size(); i++) {
        string param = args[i];

        if (param.find("-ISBN=") == 0) {
            if (paramUsed["ISBN"]) return false;
            paramUsed["ISBN"] = true;

            string newISBN = param.substr(6);
            if (!isValidISBN(newISBN) || newISBN.empty()) return false;

            if (newISBN == selectedBookISBN) return false;

            if (bookIndex.find(newISBN) != bookIndex.end()) return false;

            strncpy(book.ISBN, newISBN.c_str(), 20);
            book.ISBN[20] = '\0';
        } else if (param.find("-name=\"") == 0 && param.back() == '\"') {
            if (paramUsed["name"]) return false;
            paramUsed["name"] = true;

            string name = param.substr(7, param.length() - 8);
            if (!isValidBookName(name) || name.empty()) return false;

            strncpy(book.bookName, name.c_str(), 60);
            book.bookName[60] = '\0';
        } else if (param.find("-author=\"") == 0 && param.back() == '\"') {
            if (paramUsed["author"]) return false;
            paramUsed["author"] = true;

            string author = param.substr(9, param.length() - 10);
            if (!isValidAuthor(author) || author.empty()) return false;

            strncpy(book.author, author.c_str(), 60);
            book.author[60] = '\0';
        } else if (param.find("-keyword=\"") == 0 && param.back() == '\"') {
            if (paramUsed["keyword"]) return false;
            paramUsed["keyword"] = true;

            string keyword = param.substr(10, param.length() - 11);
            if (!isValidKeyword(keyword) || keyword.empty()) return false;

            vector<string> keywords = split(keyword, '|');
            sort(keywords.begin(), keywords.end());
            for (size_t j = 1; j < keywords.size(); j++) {
                if (keywords[j] == keywords[j-1]) return false;
            }

            strncpy(book.keyword, keyword.c_str(), 60);
            book.keyword[60] = '\0';
        } else if (param.find("-price=") == 0) {
            if (paramUsed["price"]) return false;
            paramUsed["price"] = true;

            string priceStr = param.substr(7);
            if (!isValidPrice(priceStr) || priceStr.empty()) return false;

            book.price = stod(priceStr);
        } else {
            return false;
        }
    }

    if (paramUsed["ISBN"]) {
        bookIndex.erase(selectedBookISBN);
        bookIndex[book.getISBN()] = &book - &books[0];
        selectedBookISBN = book.getISBN();
        saveSelectedBook();
    }

    saveBooks();
    return true;
}

bool importCommand(const vector<string>& args) {
    if (args.size() != 3) return false;

    if (loginStack.empty() || getCurrentPrivilege() < 3) {
        return false;
    }

    if (selectedBookISBN.empty()) {
        return false;
    }

    string qtyStr = args[1];
    string costStr = args[2];

    if (!isValidQuantity(qtyStr) || !isValidPrice(costStr)) {
        return false;
    }

    int quantity = stoi(qtyStr);
    double totalCost = stod(costStr);

    if (totalCost <= 0) {
        return false;
    }

    auto it = bookIndex.find(selectedBookISBN);
    if (it == bookIndex.end()) {
        return false;
    }

    Book& book = books[it->second];
    book.quantity += quantity;

    financeRecords.push_back(FinanceRecord(0.0, totalCost));

    saveBooks();
    saveFinance();
    return true;
}

bool showFinanceCommand(const vector<string>& args) {
    if (loginStack.empty() || getCurrentPrivilege() != ROOT_PRIVILEGE) {
        return false;
    }

    int count = -1;

    if (args.size() == 2) {
        string countStr = args[1];
        if (!isValidQuantity(countStr) && countStr != "0") {
            return false;
        }
        count = stoi(countStr);
    } else if (args.size() > 2) {
        return false;
    }

    if (count == 0) {
        cout << "\n";
        return true;
    }

    double totalIncome = 0.0;
    double totalExpenditure = 0.0;

    if (count == -1) {
        for (const auto& record : financeRecords) {
            totalIncome += record.income;
            totalExpenditure += record.expenditure;
        }
    } else {
        if (count > (int)financeRecords.size()) {
            return false;
        }

        for (int i = financeRecords.size() - count; i < (int)financeRecords.size(); i++) {
            totalIncome += financeRecords[i].income;
            totalExpenditure += financeRecords[i].expenditure;
        }
    }

    cout << "+ " << fixed << setprecision(2) << totalIncome
         << " - " << fixed << setprecision(2) << totalExpenditure << "\n";
    return true;
}

bool logCommand() {
    if (loginStack.empty() || getCurrentPrivilege() != ROOT_PRIVILEGE) {
        return false;
    }

    cout << "=== System Log ===\n";
    cout << "Total users: " << users.size() << "\n";
    cout << "Total books: " << books.size() << "\n";
    cout << "Total transactions: " << financeRecords.size() << "\n";
    cout << "Currently logged in users: " << loginStack.size() << "\n";
    return true;
}

bool reportFinanceCommand() {
    if (loginStack.empty() || getCurrentPrivilege() != ROOT_PRIVILEGE) {
        return false;
    }

    cout << "=== Financial Report ===\n";
    double totalIncome = 0.0, totalExpenditure = 0.0;
    for (const auto& record : financeRecords) {
        totalIncome += record.income;
        totalExpenditure += record.expenditure;
    }
    cout << "Total income: " << fixed << setprecision(2) << totalIncome << "\n";
    cout << "Total expenditure: " << fixed << setprecision(2) << totalExpenditure << "\n";
    cout << "Net profit: " << fixed << setprecision(2) << (totalIncome - totalExpenditure) << "\n";
    return true;
}

bool reportEmployeeCommand() {
    if (loginStack.empty() || getCurrentPrivilege() != ROOT_PRIVILEGE) {
        return false;
    }

    cout << "=== Employee Work Report ===\n";
    cout << "No employee activity recorded.\n";
    return true;
}

// Main command processor
void processCommand(const string& line) {
    if (line.empty()) return;

    vector<string> tokens;
    istringstream iss(line);
    string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) return;

    string command = tokens[0];
    bool success = false;

    if (command == "quit" || command == "exit") {
        saveAll();
        exit(0);
    } else if (command == "su") {
        success = suCommand(tokens);
    } else if (command == "logout") {
        success = logoutCommand(tokens);
    } else if (command == "register") {
        success = registerCommand(tokens);
    } else if (command == "passwd") {
        success = passwdCommand(tokens);
    } else if (command == "useradd") {
        success = useraddCommand(tokens);
    } else if (command == "delete") {
        success = deleteCommand(tokens);
    } else if (command == "show") {
        if (tokens.size() > 1 && tokens[1] == "finance") {
            success = showFinanceCommand(tokens);
        } else {
            success = showCommand(tokens);
        }
    } else if (command == "buy") {
        success = buyCommand(tokens);
    } else if (command == "select") {
        success = selectCommand(tokens);
    } else if (command == "modify") {
        success = modifyCommand(tokens);
    } else if (command == "import") {
        success = importCommand(tokens);
    } else if (command == "log") {
        success = logCommand();
    } else if (command == "report") {
        if (tokens.size() > 1) {
            if (tokens[1] == "finance") {
                success = reportFinanceCommand();
            } else if (tokens[1] == "employee") {
                success = reportEmployeeCommand();
            } else {
                success = false;
            }
        } else {
            success = false;
        }
    } else {
        success = false;
    }

    if (!success) {
        cout << "Invalid\n";
    }
}

int main() {
    // Initialize file system
    initializeFiles();

    // Load data from files
    loadUsers();
    loadBooks();
    loadFinance();
    loadLoginStack();
    loadSelectedBook();

    // Read commands from stdin
    string line;
    while (getline(cin, line)) {
        processCommand(line);
    }

    saveAll();
    return 0;
}