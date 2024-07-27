from logs_module import Logger, update_terminal
from utility import print_table
import sqlite3
import re

class Database:

    db_access = {
        "messages": 1,
        "users": 2,
        "ips": 1,
        "ports": 1,
        "conf_machine": 1
    }

    def __init__(self):
        self.connection = sqlite3.connect('covert_channel.db')
        self.create_tables()

    def create_tables(self):
        self.cursor = self.connection.cursor()
        self.cursor.execute('''CREATE TABLE IF NOT EXISTS messages (
                                id INTEGER PRIMARY KEY,
                                username TEXT,
                                message TEXT,
                                timedate TEXT,
                                station_id INT DEFAULT 1
                            )'''
        )
        self.cursor.execute('''CREATE TABLE IF NOT EXISTS users (
                                id INTEGER PRIMARY KEY,
                                username TEXT UNIQUE,
                                role TEXT,
                                station_id INT DEFAULT 1
                            )'''
        )
        self.cursor.execute('''CREATE TABLE IF NOT EXISTS conf_users (
                                id INTEGER PRIMARY KEY,
                                username TEXT UNIQUE,
                                password TEXT,
                                is_new INT,
                                user_id INT
                            )'''
        )
        self.cursor.execute('''CREATE TABLE IF NOT EXISTS ips (
                                id INTEGER PRIMARY KEY,
                                ip_name TEXT UNIQUE,
                                ip TEXT UNIQUE,
                                station_id INT DEFAULT 1
                            )'''
        )
        self.cursor.execute('''CREATE TABLE IF NOT EXISTS ports (
                                id INTEGER PRIMARY KEY,
                                port_name TEXT UNIQUE,
                                port INT UNIQUE,
                                station_id INT DEFAULT 1
                            )'''
        )
        self.cursor.execute('''CREATE TABLE IF NOT EXISTS conf_machine (
                                id INTEGER PRIMARY KEY,
                                station_name TEXT UNIQUE,
                                algorithm TEXT UNIQUE DEFAULT "DFX",
                                tsize INT DEFAULT 120
                            )'''
        )
        self.connection.commit()

    def drop_table(self, table):
        self.cursor.execute("DROP TABLE IF EXISTS {}".format(table))

    def reset_db(self, session):
        try:
            self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
            tables = self.cursor.fetchall()
            for table in tables:
                table_name = table[0]
                self.drop_table(table_name)
            self.create_tables()
            self.add_user("helicam", "master", 0, "1234")
            self.add_user("test_admin", "admin", 0, "1234")
            self.add_user("test_user", "user", 0, "1234")
            self.add_ip("vm_ip", "192.168.56.1")
            self.add_port("PORT_A", 54321)
            self.add_port("PORT_B", 54322)
            self.add_port("vm_UDP_port", 54323)
            self.add_station("Station1", "DFX", 120)
            session.user.update_user("helicam", "master")
            update_terminal(120)
            session.logger.program_log("Database reset completed successfully!", "notice")
        except sqlite3.Error:
            session.logger.program_log("Database reset failed misrably!", "error")

    def close_database(self):
        self.cursor.close()
        self.connection.close()

    #---adders----------------------------------------------------------------------------------------------------------
    def add_message(self, username, message, timedate):
        self.cursor.execute("INSERT INTO messages (username, message, timedate) VALUES (?, ?, ?)",\
                            (username, message, timedate))
        self.connection.commit()

    def add_ip(self, ip_name, ip):
        self.cursor.execute("INSERT INTO ips (ip_name, ip) VALUES (?, ?)",\
                            (ip_name, ip))
        self.connection.commit()

    def add_port(self, port_name, port):
        self.cursor.execute("INSERT INTO ports (port_name, port) VALUES (?, ?)",\
                            (port_name, port))
        self.connection.commit()

    def add_station(self, name, algorithm, tsize):
        self.cursor.execute("INSERT INTO conf_machine (station_name, algorithm, tsize) VALUES (?, ?, ?)",\
                            (name, algorithm, tsize))
        self.connection.commit()

    #---getters---------------------------------------------------------------------------------------------------------
    def get_terminal_size(self, station=1):
        query = f'SELECT tsize FROM conf_machine WHERE id = ?'
        self.cursor.execute(query, (station,))
        return self.cursor.fetchone()[0]

    def get_ips(self, station=1):
        query = f'SELECT ip_name, ip FROM ips WHERE station_id = ?'
        self.cursor.execute(query, (station,))
        ips = {}
        for row in self.cursor.fetchall():
            ip_name, ip = row
            ips[ip_name] = ip
        return ips

    def get_ports(self, station=1):
        query = f'SELECT port_name, port FROM ports WHERE station_id = ?'
        self.cursor.execute(query, (station,))
        ports = {}
        for row in self.cursor.fetchall():
            port_name, port = row
            ports[port_name] = port
        return ports

    def fetch_table(self, session, table_name):
        if table_name != "conf_users" and Database.db_access[table_name] <= session.user.get_access():
            if table_name == "messages" and session.user.get_access() < 2:
                username = session.user.get_name()
                query = f"SELECT * FROM 'messages' WHERE username = ?"
                self.cursor.execute(query, (username,))
            else:
                self.cursor.execute("SELECT * FROM {}".format(table_name))
            column_names = [description[0] for description in self.cursor.description]
            rows = self.cursor.fetchall()
            print_table(column_names, rows, table_name)
        else:
            session.logger.program_log("Access denied", "error")

    def fetch_all_tables(self, session):
        self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
        tables = self.cursor.fetchall()
        for table in tables:
            table_name = table[0]
            if table_name != "conf_users" and Database.db_access[table_name] <= session.user.get_access():
                if table_name == "messages" and session.user.get_access() < 2:
                    username = session.user.get_name()
                    query = f"SELECT * FROM 'messages' WHERE username = ?"
                    self.cursor.execute(query, (username,))
                else:
                    self.cursor.execute(f"SELECT * FROM {table_name}")
                rows = self.cursor.fetchall()
                column_names = [description[0] for description in self.cursor.description]
                print_table(column_names, rows, table_name)

    #---users-----------------------------------------------------------------------------------------------------------
    def find_user(self, username):
        query = f'SELECT id, username, role, station_id FROM users WHERE username = ?'
        self.cursor.execute(query, (username,))
        return self.cursor.fetchone()

    def find_conf(self, user_id):
        query = f'SELECT password, is_new FROM conf_users WHERE user_id = ?'
        self.cursor.execute(query, (user_id,))
        return self.cursor.fetchone()

    def update_password(self, session, user_id, new_password):
        query = f'UPDATE conf_users SET password = ?, is_new = 0 WHERE user_id = ?'
        self.cursor.execute(query, (new_password, user_id))
        self.connection.commit()
        session.logger.program_log(f"Password updated!", "notice")

    def add_user(self, username, role, is_new, password="0000", station=1):
        self.cursor.execute("INSERT INTO users (username, role, station_id) VALUES (?, ?, ?)", \
                            (username, role, station))
        user_id = self.find_user(username)[0]
        self.cursor.execute("INSERT INTO conf_users (username, password, is_new, user_id) VALUES (?, ?, ?, ?)", \
                            (username, password, is_new, user_id))
        self.connection.commit()

    def remove_user(self, session, username, user_role):
        if session.user.authority_check(user_role):
            query = f'DELETE FROM users WHERE username = ?'
            self.cursor.execute(query, (username,))
            self.connection.commit()
            query = f'DELETE FROM conf_users WHERE username = ?'
            self.cursor.execute(query, (username,))
            self.connection.commit()
            session.logger.program_log(f"User {username} has been removed", "notice")
        else:
            session.logger.program_log(f"User {username} cannot be removed", "error")

    def promote_user(self, session, username):
        try:
            query = f'SELECT role FROM users WHERE username = ?'
            self.cursor.execute(query, (username,))
            curr_role = self.cursor.fetchone()[0]
            if curr_role in ["admin", "master"]:
                session.logger.program_log(f"The user {username} is already an admin", "notice")
            elif curr_role == "user":
                query = f'UPDATE users SET role = ? WHERE username = ?'
                self.cursor.execute(query, ("admin", username))
                self.connection.commit()
                session.logger.program_log(f"The user {username} promoted to admin!", "notice")
            else:
                session.logger.program_log(f"User {username} not found", "error")
        except sqlite3.Error as e:
            print(f"An error occurred: {str(e)}")

    #---updates---------------------------------------------------------------------------------------------------------
    def update_terminal_size(self, session, new_size, station=1):
        try:
            if new_size < Logger.PROGRAM_LOG_LENGTH_MIN or new_size > Logger.PROGRAM_LOG_LENGTH_MAX:
                raise ValueError
            username = session.user.get_name()
            query = f'UPDATE conf_machine SET tsize = ? WHERE id = ?'
            self.cursor.execute(query, (new_size,station))
            self.connection.commit()
            update_terminal(new_size)
            session.logger.program_log("Terminal size updated", "notice")
        except ValueError:
            session.logger.program_log("Terminal size must be between 120 and 170!", "error")
        except sqlite3.Error as e:
            session.logger.program_log(str(e), "error")

    def update_algorithm(self, session, new_algorithm, station=1):
        try:
            query = f'UPDATE conf_machine SET algorithm = ? WHERE id = ?'
            self.cursor.execute(query, (new_algorithm,station))
            self.connection.commit()
            session.logger.program_log("Algorithm updated", "notice")
        except sqlite3.Error as e:
            session.logger.program_log(str(e), "error")

    def edit_table(self, session, name, table_name, input):
        try:
            ip_pattern = r'^([0-9]{1,3}(\.[0-9]{1,3}){3})$'
            port_pattern = r'^([0-9]+)$'
            item = table_name[:-1]
            item_name = item + "_name"
            if item == "ip":
                if not bool(re.match(ip_pattern, input)):
                    raise SyntaxError("Wrong ip syntax")
            elif item == "port":
                if not bool(re.match(port_pattern, input)):
                    raise SyntaxError("Wrong port syntax")
            query = f'UPDATE {table_name} SET {item} = ? WHERE {item_name} = ?'
            self.cursor.execute(query, (input, name))
            self.connection.commit()
            session.logger.program_log(f"{item.capitalize()} {name} updated", "notice")
        except sqlite3.Error as e:
            session.logger.program_log(str(e), "error")
        except SyntaxError as e:
            session.logger.program_log(str(e), "error")
