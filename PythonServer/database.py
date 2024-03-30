import sqlite3

class Database:

    def __init__(self, logger):
        self.logger = logger
        self.connection = sqlite3.connect('covert_channel.db')
        self.cursor = self.connection.cursor()
        self.cursor.execute('''CREATE TABLE IF NOT EXISTS messages (
                            id INTEGER PRIMARY KEY,
                            username TEXT,
                            message TEXT,
                            timedate TEXT
                        )''')

    def add_message(self, username, message, timedate):
        self.cursor.execute("INSERT INTO messages (username, message, timedate) VALUES (?, ?, ?)",\
                            (username, message, timedate))
        self.connection.commit()

    def fetch_table(self, table):
        self.logger.print_title(table)
        self.cursor.execute("SELECT * FROM {}".format(table))
        column_names = [description[0] for description in self.cursor.description]
        rows = self.cursor.fetchall()
        self.logger.print_table(column_names, rows)

    def fetch_all_tables(self):
        self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
        tables = self.cursor.fetchall()
        for table in tables:
            print(table[0])

    def drop_table(self, table):
        self.cursor.execute("DROP TABLE IF EXISTS {}".format(table))

    def close_database(self):
        self.cursor.close()
        self.connection.close()
