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

using namespace std;

// Constants
const string ROOT_USERNAME = "root";
const string ROOT_PASSWORD = "sjtu";
const int ROOT_PRIVILEGE = 7;

// Data structures
struct User {
    string userID;
    string password;
    string username;
    int privilege;

    User() : privilege(0) {}
    User(string id, string pass, string name, int priv)
        : userID(id), password(pass), username(name), privilege(priv) {}
};

struct Book {
    string ISBN;
    string bookName;
    string author;
    string keyword;
    double price;
    int quantity;

    Book() : price(0.0), quantity(0) {}
    Book(string isbn) : ISBN(isbn), price(0.0), quantity(0) {}
};

// Global state
vector<User> users;
vector<Book> books;
vector<User> loginStack;
Book* selectedBook = nullptr;
map<string, int> userIndex; // userID -> index in users
map<string, int> bookIndex; // ISBN -> index in books

// Financial records
vector<pair<double, double>> financeRecords; // (income, expenditure)

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
        if (c < 32 || c > 126) return false; // ASCII printable characters only
    }
    return true;
}

bool isValidISBN(const string& isbn) {
    if (isbn.empty() || isbn.length() > 20) return false;
    for (char c : isbn) {
        if (c < 32 || c > 126) return false; // ASCII printable characters only
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

    // Check if it has at most 2 decimal places
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

// System initialization
void initializeSystem() {
    // Check if root user already exists
    bool rootExists = false;
    for (const auto& user : users) {
        if (user.userID == ROOT_USERNAME) {
            rootExists = true;
            break;
        }
    }

    if (!rootExists) {
        User root(ROOT_USERNAME, ROOT_PASSWORD, "superadmin", ROOT_PRIVILEGE);
        users.push_back(root);
        userIndex[ROOT_USERNAME] = users.size() - 1;
    }
}

// Command implementations
bool suCommand(const vector<string>& args) {
    if (args.size() < 2 || args.size() > 3) return false;

    string userID = args[1];
    string password = "";

    if (args.size() == 3) {
        password = args[2];
    }

    // Find user
    auto it = userIndex.find(userID);
    if (it == userIndex.end()) {
        return false; // User doesn't exist
    }

    User& user = users[it->second];

    // Check if password is required
    if (password.empty()) {
        // Password can be omitted only if current privilege > target privilege
        if (loginStack.empty() || loginStack.back().privilege <= user.privilege) {
            return false;
        }
    } else {
        // Check password
        if (user.password != password) {
            return false;
        }
    }

    // Login successful
    loginStack.push_back(user);
    selectedBook = nullptr; // Clear selected book when switching users
    return true;
}

bool logoutCommand(const vector<string>& args) {
    if (args.size() != 1) return false;

    if (loginStack.empty()) {
        return false;
    }

    loginStack.pop_back();
    selectedBook = nullptr; // Clear selected book on logout
    return true;
}

bool registerCommand(const vector<string>& args) {
    if (args.size() != 4) return false;

    string userID = args[1];
    string password = args[2];
    string username = args[3];

    // Validate inputs
    if (!isValidUserID(userID) || !isValidPassword(password) || !isValidUsername(username)) {
        return false;
    }

    // Check if user already exists
    if (userIndex.find(userID) != userIndex.end()) {
        return false;
    }

    // Create new user with privilege 1
    User newUser(userID, password, username, 1);
    users.push_back(newUser);
    userIndex[userID] = users.size() - 1;
    return true;
}

bool passwdCommand(const vector<string>& args) {
    if (args.size() < 3 || args.size() > 4) return false;

    string userID = args[1];
    string currentPassword = "";
    string newPassword = "";

    if (args.size() == 3) {
        // Format: passwd [UserID] [NewPassword]
        newPassword = args[2];

        // Current user must be root to omit current password
        if (loginStack.empty() || loginStack.back().privilege != ROOT_PRIVILEGE) {
            return false;
        }
    } else {
        // Format: passwd [UserID] [CurrentPassword] [NewPassword]
        currentPassword = args[2];
        newPassword = args[3];
    }

    // Find user
    auto it = userIndex.find(userID);
    if (it == userIndex.end()) {
        return false; // User doesn't exist
    }

    User& user = users[it->second];

    // Check current password if provided
    if (!currentPassword.empty() && user.password != currentPassword) {
        return false;
    }

    // Validate new password
    if (!isValidPassword(newPassword)) {
        return false;
    }

    // Update password
    user.password = newPassword;
    return true;
}

bool useraddCommand(const vector<string>& args) {
    if (args.size() != 5) return false;

    // Check current privilege
    if (loginStack.empty() || loginStack.back().privilege < 3) {
        return false;
    }

    string userID = args[1];
    string password = args[2];
    string privilegeStr = args[3];
    string username = args[4];

    // Validate inputs
    if (!isValidUserID(userID) || !isValidPassword(password) ||
        !isValidPrivilege(privilegeStr) || !isValidUsername(username)) {
        return false;
    }

    int privilege = getPrivilegeFromStr(privilegeStr);

    // Check privilege rules
    if (privilege >= loginStack.back().privilege) {
        return false;
    }

    // Check if user already exists
    if (userIndex.find(userID) != userIndex.end()) {
        return false;
    }

    // Create new user
    User newUser(userID, password, username, privilege);
    users.push_back(newUser);
    userIndex[userID] = users.size() - 1;
    return true;
}

bool deleteCommand(const vector<string>& args) {
    if (args.size() != 2) return false;

    // Check current privilege
    if (loginStack.empty() || loginStack.back().privilege != ROOT_PRIVILEGE) {
        return false;
    }

    string userID = args[1];

    // Find user
    auto it = userIndex.find(userID);
    if (it == userIndex.end()) {
        return false; // User doesn't exist
    }

    // Check if user is logged in
    for (const auto& loggedInUser : loginStack) {
        if (loggedInUser.userID == userID) {
            return false;
        }
    }

    // Remove user
    int index = it->second;
    users.erase(users.begin() + index);
    userIndex.erase(it);

    // Update indices
    for (auto& pair : userIndex) {
        if (pair.second > index) {
            pair.second--;
        }
    }

    return true;
}

bool showCommand(const vector<string>& args) {
    if (loginStack.empty() || loginStack.back().privilege < 1) {
        return false;
    }

    vector<Book> results;

    if (args.size() == 1) {
        // Show all books
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
                if (book.bookName == name) {
                    results.push_back(book);
                }
            }
        } else if (param.find("-author=\"") == 0 && param.back() == '\"') {
            string author = param.substr(9, param.length() - 10);
            if (!isValidAuthor(author) || author.empty()) return false;

            for (const auto& book : books) {
                if (book.author == author) {
                    results.push_back(book);
                }
            }
        } else if (param.find("-keyword=\"") == 0 && param.back() == '\"') {
            string keyword = param.substr(10, param.length() - 11);
            if (!isValidKeyword(keyword) || keyword.empty()) return false;

            // Check if keyword contains multiple keywords (not allowed for show)
            if (keyword.find('|') != string::npos) {
                return false;
            }

            for (const auto& book : books) {
                if (book.keyword.find(keyword) != string::npos) {
                    // Check if keyword matches exactly (considering | separators)
                    vector<string> keywords = split(book.keyword, '|');
                    for (const auto& kw : keywords) {
                        if (kw == keyword) {
                            results.push_back(book);
                            break;
                        }
                    }
                }
            }
        } else {
            return false;
        }
    } else {
        return false;
    }

    // Sort results by ISBN
    sort(results.begin(), results.end(), [](const Book& a, const Book& b) {
        return a.ISBN < b.ISBN;
    });

    // Output results
    for (const auto& book : results) {
        cout << book.ISBN << "\t" << book.bookName << "\t" << book.author
             << "\t" << book.keyword << "\t" << fixed << setprecision(2)
             << book.price << "\t" << book.quantity << "\n";
    }

    if (results.empty()) {
        cout << "\n";
    }

    return true;
}

bool buyCommand(const vector<string>& args) {
    if (args.size() != 3) return false;

    if (loginStack.empty() || loginStack.back().privilege < 1) {
        return false;
    }

    string isbn = args[1];
    string qtyStr = args[2];

    if (!isValidISBN(isbn) || !isValidQuantity(qtyStr)) {
        return false;
    }

    int quantity = stoi(qtyStr);

    // Find book
    auto it = bookIndex.find(isbn);
    if (it == bookIndex.end()) {
        return false; // Book doesn't exist
    }

    Book& book = books[it->second];

    // Check inventory
    if (book.quantity < quantity) {
        return false;
    }

    // Calculate total cost
    double totalCost = book.price * quantity;

    // Update inventory
    book.quantity -= quantity;

    // Record transaction
    financeRecords.push_back({totalCost, 0.0});

    // Output total cost
    cout << fixed << setprecision(2) << totalCost << "\n";
    return true;
}

bool selectCommand(const vector<string>& args) {
    if (args.size() != 2) return false;

    if (loginStack.empty() || loginStack.back().privilege < 3) {
        return false;
    }

    string isbn = args[1];

    if (!isValidISBN(isbn)) {
        return false;
    }

    // Find or create book
    auto it = bookIndex.find(isbn);
    if (it == bookIndex.end()) {
        // Create new book
        Book newBook(isbn);
        books.push_back(newBook);
        bookIndex[isbn] = books.size() - 1;
        selectedBook = &books.back();
    } else {
        selectedBook = &books[it->second];
    }

    return true;
}

bool modifyCommand(const vector<string>& args) {
    if (args.size() < 2) return false;

    if (loginStack.empty() || loginStack.back().privilege < 3) {
        return false;
    }

    if (!selectedBook) {
        return false; // No book selected
    }

    Book originalBook = *selectedBook;
    map<string, bool> paramUsed;

    for (size_t i = 1; i < args.size(); i++) {
        string param = args[i];

        if (param.find("-ISBN=") == 0) {
            if (paramUsed["ISBN"]) return false; // Duplicate parameter
            paramUsed["ISBN"] = true;

            string newISBN = param.substr(6);
            if (!isValidISBN(newISBN) || newISBN.empty()) return false;

            // Cannot change to same ISBN
            if (newISBN == selectedBook->ISBN) return false;

            // Check if new ISBN already exists
            if (bookIndex.find(newISBN) != bookIndex.end()) return false;

            selectedBook->ISBN = newISBN;
        } else if (param.find("-name=\"") == 0 && param.back() == '\"') {
            if (paramUsed["name"]) return false;
            paramUsed["name"] = true;

            string name = param.substr(7, param.length() - 8);
            if (!isValidBookName(name) || name.empty()) return false;

            selectedBook->bookName = name;
        } else if (param.find("-author=\"") == 0 && param.back() == '\"') {
            if (paramUsed["author"]) return false;
            paramUsed["author"] = true;

            string author = param.substr(9, param.length() - 10);
            if (!isValidAuthor(author) || author.empty()) return false;

            selectedBook->author = author;
        } else if (param.find("-keyword=\"") == 0 && param.back() == '\"') {
            if (paramUsed["keyword"]) return false;
            paramUsed["keyword"] = true;

            string keyword = param.substr(10, param.length() - 11);
            if (!isValidKeyword(keyword) || keyword.empty()) return false;

            // Check for duplicate keywords
            vector<string> keywords = split(keyword, '|');
            sort(keywords.begin(), keywords.end());
            for (size_t j = 1; j < keywords.size(); j++) {
                if (keywords[j] == keywords[j-1]) return false;
            }

            selectedBook->keyword = keyword;
        } else if (param.find("-price=") == 0) {
            if (paramUsed["price"]) return false;
            paramUsed["price"] = true;

            string priceStr = param.substr(7);
            if (!isValidPrice(priceStr) || priceStr.empty()) return false;

            selectedBook->price = stod(priceStr);
        } else {
            return false;
        }
    }

    // Update book index if ISBN changed
    if (paramUsed["ISBN"]) {
        bookIndex.erase(originalBook.ISBN);
        bookIndex[selectedBook->ISBN] = &(*selectedBook) - &books[0];
    }

    return true;
}

bool importCommand(const vector<string>& args) {
    if (args.size() != 3) return false;

    if (loginStack.empty() || loginStack.back().privilege < 3) {
        return false;
    }

    if (!selectedBook) {
        return false; // No book selected
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

    // Update inventory
    selectedBook->quantity += quantity;

    // Record transaction
    financeRecords.push_back({0.0, totalCost});

    return true;
}

bool showFinanceCommand(const vector<string>& args) {
    if (loginStack.empty() || loginStack.back().privilege != ROOT_PRIVILEGE) {
        return false;
    }

    int count = -1; // -1 means all

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
        // All records
        for (const auto& record : financeRecords) {
            totalIncome += record.first;
            totalExpenditure += record.second;
        }
    } else {
        // Last 'count' records
        if (count > (int)financeRecords.size()) {
            return false;
        }

        for (int i = financeRecords.size() - count; i < (int)financeRecords.size(); i++) {
            totalIncome += financeRecords[i].first;
            totalExpenditure += financeRecords[i].second;
        }
    }

    cout << "+ " << fixed << setprecision(2) << totalIncome
         << " - " << fixed << setprecision(2) << totalExpenditure << "\n";
    return true;
}

bool logCommand() {
    if (loginStack.empty() || loginStack.back().privilege != ROOT_PRIVILEGE) {
        return false;
    }

    // Simple log implementation
    cout << "=== System Log ===\n";
    cout << "Total users: " << users.size() << "\n";
    cout << "Total books: " << books.size() << "\n";
    cout << "Total transactions: " << financeRecords.size() << "\n";
    cout << "Currently logged in users: " << loginStack.size() << "\n";

    return true;
}

bool reportFinanceCommand() {
    if (loginStack.empty() || loginStack.back().privilege != ROOT_PRIVILEGE) {
        return false;
    }

    cout << "=== Financial Report ===\n";
    cout << "Total income: " << fixed << setprecision(2);
    double totalIncome = 0.0;
    for (const auto& record : financeRecords) {
        totalIncome += record.first;
    }
    cout << totalIncome << "\n";

    cout << "Total expenditure: " << fixed << setprecision(2);
    double totalExpenditure = 0.0;
    for (const auto& record : financeRecords) {
        totalExpenditure += record.second;
    }
    cout << totalExpenditure << "\n";

    cout << "Net profit: " << fixed << setprecision(2)
         << (totalIncome - totalExpenditure) << "\n";

    return true;
}

bool reportEmployeeCommand() {
    if (loginStack.empty() || loginStack.back().privilege != ROOT_PRIVILEGE) {
        return false;
    }

    cout << "=== Employee Work Report ===\n";
    cout << "No employee activity recorded in this simple implementation.\n";

    return true;
}

// Main command processor
void processCommand(const string& line) {
    if (line.empty()) return;

    // Split command into tokens
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
    // Initialize system
    initializeSystem();

    // Read commands from stdin
    string line;
    while (getline(cin, line)) {
        processCommand(line);
    }

    return 0;
}