#include <iostream>
#include <iomanip>
#include <memory>
#include <limits>
#include <string>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

using namespace std;

const string DB_HOST = "tcp://127.0.0.1:3306";
const string DB_USER = "root";
const string DB_PASSWORD = "YOUR_MYSQL_PASSWORD";
const string DB_NAME = "library_db";

class LibraryManagementSystem {
private:
    sql::mysql::MySQL_Driver* driver;
    unique_ptr<sql::Connection> con;

    void clearInput() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    int readInt(const string& prompt) {
        int value;
        while (true) {
            cout << prompt;
            if (cin >> value) {
                clearInput();
                return value;
            }
            cout << "Please enter a valid number.\n";
            clearInput();
        }
    }

public:
    LibraryManagementSystem() {
        driver = sql::mysql::get_mysql_driver_instance();
        con.reset(driver->connect(DB_HOST, DB_USER, DB_PASSWORD));
        con->setSchema(DB_NAME);
    }

    void addBook() {
        string title, author, isbn;
        int quantity;

        cout << "Book title: ";
        getline(cin, title);
        cout << "Author: ";
        getline(cin, author);
        cout << "ISBN: ";
        getline(cin, isbn);
        quantity = readInt("Quantity: ");

        if (title.empty() || author.empty() || isbn.empty() || quantity < 0) {
            cout << "Invalid book details.\n";
            return;
        }

        unique_ptr<sql::PreparedStatement> stmt(
            con->prepareStatement(
                "INSERT INTO books (title, author, isbn, total_copies, available_copies) "
                "VALUES (?, ?, ?, ?, ?)"
            )
        );
        stmt->setString(1, title);
        stmt->setString(2, author);
        stmt->setString(3, isbn);
        stmt->setInt(4, quantity);
        stmt->setInt(5, quantity);
        stmt->executeUpdate();

        cout << "Book added successfully.\n";
    }

    void viewBooks() {
        unique_ptr<sql::Statement> stmt(con->createStatement());
        unique_ptr<sql::ResultSet> res(
            stmt->executeQuery(
                "SELECT book_id, title, author, isbn, total_copies, available_copies "
                "FROM books ORDER BY book_id"
            )
        );

        cout << "\n---------------- BOOK LIST ----------------\n";
        cout << left << setw(7) << "ID" << setw(28) << "TITLE"
             << setw(22) << "AUTHOR" << setw(16) << "ISBN"
             << setw(9) << "TOTAL" << "AVAILABLE\n";

        while (res->next()) {
            cout << left << setw(7) << res->getInt("book_id")
                 << setw(28) << res->getString("title").substr(0, 26)
                 << setw(22) << res->getString("author").substr(0, 20)
                 << setw(16) << res->getString("isbn")
                 << setw(9) << res->getInt("total_copies")
                 << res->getInt("available_copies") << '\n';
        }
    }

    void searchBook() {
        string keyword;
        cout << "Enter title, author, or ISBN: ";
        getline(cin, keyword);

        unique_ptr<sql::PreparedStatement> stmt(
            con->prepareStatement(
                "SELECT book_id, title, author, isbn, available_copies "
                "FROM books WHERE title LIKE ? OR author LIKE ? OR isbn LIKE ?"
            )
        );
        string pattern = "%" + keyword + "%";
        stmt->setString(1, pattern);
        stmt->setString(2, pattern);
        stmt->setString(3, pattern);

        unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        bool found = false;
        while (res->next()) {
            found = true;
            cout << "\nID: " << res->getInt("book_id")
                 << "\nTitle: " << res->getString("title")
                 << "\nAuthor: " << res->getString("author")
                 << "\nISBN: " << res->getString("isbn")
                 << "\nAvailable copies: " << res->getInt("available_copies")
                 << "\n";
        }
        if (!found) cout << "No matching book found.\n";
    }

    void addMember() {
        string name, email, phone;
        cout << "Member name: ";
        getline(cin, name);
        cout << "Email: ";
        getline(cin, email);
        cout << "Phone: ";
        getline(cin, phone);

        if (name.empty() || email.empty()) {
            cout << "Name and email are required.\n";
            return;
        }

        unique_ptr<sql::PreparedStatement> stmt(
            con->prepareStatement(
                "INSERT INTO members (name, email, phone) VALUES (?, ?, ?)"
            )
        );
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, phone);
        stmt->executeUpdate();

        cout << "Member registered successfully.\n";
    }

    void viewMembers() {
        unique_ptr<sql::Statement> stmt(con->createStatement());
        unique_ptr<sql::ResultSet> res(
            stmt->executeQuery(
                "SELECT member_id, name, email, phone FROM members ORDER BY member_id"
            )
        );

        cout << "\n-------------- MEMBER LIST --------------\n";
        while (res->next()) {
            cout << "ID: " << res->getInt("member_id")
                 << " | Name: " << res->getString("name")
                 << " | Email: " << res->getString("email")
                 << " | Phone: " << res->getString("phone") << '\n';
        }
    }

    void issueBook() {
        int bookId = readInt("Book ID: ");
        int memberId = readInt("Member ID: ");

        con->setAutoCommit(false);
        try {
            unique_ptr<sql::PreparedStatement> checkBook(
                con->prepareStatement(
                    "SELECT available_copies FROM books WHERE book_id = ? FOR UPDATE"
                )
            );
            checkBook->setInt(1, bookId);
            unique_ptr<sql::ResultSet> book(checkBook->executeQuery());

            if (!book->next() || book->getInt("available_copies") < 1) {
                con->rollback();
                con->setAutoCommit(true);
                cout << "Book not found or no copies are available.\n";
                return;
            }

            unique_ptr<sql::PreparedStatement> checkMember(
                con->prepareStatement("SELECT member_id FROM members WHERE member_id = ?")
            );
            checkMember->setInt(1, memberId);
            unique_ptr<sql::ResultSet> member(checkMember->executeQuery());

            if (!member->next()) {
                con->rollback();
                con->setAutoCommit(true);
                cout << "Member not found.\n";
                return;
            }

            unique_ptr<sql::PreparedStatement> issue(
                con->prepareStatement(
                    "INSERT INTO issued_books (book_id, member_id, issue_date, due_date) "
                    "VALUES (?, ?, CURDATE(), DATE_ADD(CURDATE(), INTERVAL 14 DAY))"
                )
            );
            issue->setInt(1, bookId);
            issue->setInt(2, memberId);
            issue->executeUpdate();

            unique_ptr<sql::PreparedStatement> update(
                con->prepareStatement(
                    "UPDATE books SET available_copies = available_copies - 1 "
                    "WHERE book_id = ?"
                )
            );
            update->setInt(1, bookId);
            update->executeUpdate();

            con->commit();
            con->setAutoCommit(true);
            cout << "Book issued successfully. Due date is in 14 days.\n";
        } catch (...) {
            con->rollback();
            con->setAutoCommit(true);
            throw;
        }
    }

    void returnBook() {
        int issueId = readInt("Issue/Loan ID: ");

        con->setAutoCommit(false);
        try {
            unique_ptr<sql::PreparedStatement> findLoan(
                con->prepareStatement(
                    "SELECT book_id FROM issued_books "
                    "WHERE issue_id = ? AND return_date IS NULL FOR UPDATE"
                )
            );
            findLoan->setInt(1, issueId);
            unique_ptr<sql::ResultSet> loan(findLoan->executeQuery());

            if (!loan->next()) {
                con->rollback();
                con->setAutoCommit(true);
                cout << "Active loan not found.\n";
                return;
            }
            int bookId = loan->getInt("book_id");

            unique_ptr<sql::PreparedStatement> closeLoan(
                con->prepareStatement(
                    "UPDATE issued_books SET return_date = CURDATE() WHERE issue_id = ?"
                )
            );
            closeLoan->setInt(1, issueId);
            closeLoan->executeUpdate();

            unique_ptr<sql::PreparedStatement> updateBook(
                con->prepareStatement(
                    "UPDATE books SET available_copies = available_copies + 1 "
                    "WHERE book_id = ?"
                )
            );
            updateBook->setInt(1, bookId);
            updateBook->executeUpdate();

            con->commit();
            con->setAutoCommit(true);
            cout << "Book returned successfully.\n";
        } catch (...) {
            con->rollback();
            con->setAutoCommit(true);
            throw;
        }
    }

    void viewActiveLoans() {
        unique_ptr<sql::Statement> stmt(con->createStatement());
        unique_ptr<sql::ResultSet> res(
            stmt->executeQuery(
                "SELECT i.issue_id, b.title, m.name, i.issue_date, i.due_date "
                "FROM issued_books i "
                "JOIN books b ON i.book_id = b.book_id "
                "JOIN members m ON i.member_id = m.member_id "
                "WHERE i.return_date IS NULL ORDER BY i.due_date"
            )
        );

        cout << "\n--------------- ACTIVE LOANS ---------------\n";
        while (res->next()) {
            cout << "Loan ID: " << res->getInt("issue_id")
                 << " | Book: " << res->getString("title")
                 << " | Member: " << res->getString("name")
                 << " | Issued: " << res->getString("issue_date")
                 << " | Due: " << res->getString("due_date") << '\n';
        }
    }

    void run() {
        int choice;
        do {
            cout << "\n========== LIBRARY MANAGEMENT SYSTEM ==========\n"
                 << "1. Add book\n"
                 << "2. View all books\n"
                 << "3. Search book\n"
                 << "4. Register member\n"
                 << "5. View members\n"
                 << "6. Issue book\n"
                 << "7. Return book\n"
                 << "8. View active loans\n"
                 << "0. Exit\n";

            choice = readInt("Enter choice: ");
            switch (choice) {
                case 1: addBook(); break;
                case 2: viewBooks(); break;
                case 3: searchBook(); break;
                case 4: addMember(); break;
                case 5: viewMembers(); break;
                case 6: issueBook(); break;
                case 7: returnBook(); break;
                case 8: viewActiveLoans(); break;
                case 0: cout << "Thank you for using the system.\n"; break;
                default: cout << "Invalid choice. Try again.\n";
            }
        } while (choice != 0);
    }
};

int main() {
    try {
        LibraryManagementSystem system;
        cout << "Connected to MySQL successfully.\n";
        system.run();
    } catch (sql::SQLException& e) {
        cerr << "MySQL error: " << e.what()
             << "\nError code: " << e.getErrorCode()
             << "\nSQLState: " << e.getSQLState() << '\n';
        return 1;
    } catch (exception& e) {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
