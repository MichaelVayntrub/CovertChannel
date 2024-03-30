
class User:

    Roles = {
        "user": 0,
        "admin": 1,
        "manager": 2
    }

    def __init__(self, role, username):
        self.role = role
        self.username = username

    def get_access(self):
        return User.Roles[self.role]
