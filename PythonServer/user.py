from input_module import Input
from logs_module import update_terminal
import getpass
import re

class User:
    pattern_user = r'^[a-zA-Z0-9_]+$'
    pattern_pass = r'^[a-zA-Z0-9]+$'

    Roles = {
        "guest": 0,
        "user": 1,
        "admin": 2,
        "master": 3
    }

    def __init__(self, role, username, tsize=120):
        self.role = role
        self.username = username

    def login(self, session):
        try:
            username = input(session.logger.request_format("Enter username: "))
            if username is None or not bool(re.match(User.pattern_user, username)):
                raise SyntaxError("Username")
            if len(username) < 3:
                raise ValueError("At least 3 characters required")
            user = session.db.find_user(username)
            if not user:
                raise ValueError("User not found")
            user_id = user[0]

            password_db, is_new = session.db.find_conf(user_id)
            if is_new:
                new_password = getpass.getpass(prompt=session.logger.request_format("Enter new password: "))
                session.db.update_password(session, user_id, new_password)
            else:
                password = getpass.getpass(prompt=session.logger.request_format("Enter password: "))
                if password is None or not bool(re.match(User.pattern_pass, password)):
                    raise SyntaxError("Password")
                if len(username) < 3:
                    raise ValueError("At least 3 characters required")
                if password_db != password:
                    raise ValueError("Password incorrect")

            self.update_user(username, user[2])
            session.logger.program_log(f"Welcome {username}!", "notice")

        except SyntaxError as e:
            session.logger.program_log(f"{str(e)} syntax isn't valid", "error")
        except ValueError as e:
            session.logger.program_log(str(e), "error")

    def update_user(self, username, role):
        self.username = username
        self.role = role

    def get_name(self):
        return self.username

    def get_access(self):
        return User.Roles[self.role]

    def authority_check(self, role):
        return self.get_access() > User.Roles[role]

def input_username_check(session):
    username = input(session.logger.request_format("Enter username: "))
    if username is None or not bool(re.match(User.pattern_user, username)):
        raise SyntaxError("Username syntax isn't valid")
    if len(username) < 3:
        raise ValueError("At least 3 characters required")
    return username, session.db.find_user(username)
