#XLSXtoSQL

This project is going to be a simple command line program to take data from a Microsoft excel spreadsheet and import it into a mysql database.

#Current State

Right now the program reads a XLSX file into memory, then constructs sql queries to push it to a mysql database. It is limited to only reading
one sheet right now, but I am working to implement it so that it can read multiple sheets and put their associated data into tables named
after each sheet's name.
