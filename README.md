# Library Management System (C++ and MySQL)

A menu-driven console application for managing a small library. It uses C++ for application logic and MySQL for persistent storage.

## Features
- Add, list, and search books
- Register and list library members
- Issue books with a 14-day due date
- Return books and update available stock
- View active loans
- Prepared statements for user input
- SQL transactions for issue/return operations

## Database setup
1. Install MySQL Server and start the MySQL service.
2. Open MySQL Workbench or the MySQL command line.
3. Run `database/schema.sql`.
4. In `src/main.cpp`, update `DB_USER` and `DB_PASSWORD` to match your MySQL account.

## C++ dependency
Install **MySQL Connector/C++** (the classic JDBC API package). Configure your compiler to include the Connector/C++ `include` directory and link its `mysqlcppconn` library.

The exact include/library paths depend on where Connector/C++ is installed and on your compiler version.

## Build example (Linux)
The following is an example only; adjust paths and library names to your installation:

```bash
g++ -std=c++17 src/main.cpp -o library_app \
  -I/usr/include -lmysqlcppconn
./library_app
```

## Build on Windows (MinGW)
1. Install MySQL Connector/C++ compatible with your MinGW toolchain.
2. Add the connector's `include` folder to the compiler include paths.
3. Add its library folder to the linker library paths and link the connector library (commonly `mysqlcppconn`).
4. Ensure the connector DLL and its dependencies are available on your Windows `PATH` when running the program.

Connector packages differ by version and compiler ABI, so use the build instructions shipped with your installed Connector/C++ package. A connector built for MSVC may not link with MinGW.

## Important
- This project expects MySQL Connector/C++'s classic API headers (`mysql_driver.h`, `mysql_connection.h`, and `cppconn/...`).
- Do not commit real database passwords to a public GitHub repository. For a portfolio version, read credentials from environment variables or a local configuration file excluded by `.gitignore`.
- This is a learning/resume project, not a production library system. It does not yet implement login roles, fines, reservations, or a graphical interface.
