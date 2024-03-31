from prettytable import PrettyTable, DOUBLE_BORDER
from colorama import Fore, Style, Back
from datetime import datetime
import msvcrt
import re


def clear_buffer():
    while msvcrt.kbhit():
        msvcrt.getch()

class Input:

    INPUT_PREFIX = Style.BRIGHT + "\n (--Covert_Channel--@{}<{}>) >> " + Style.RESET_ALL
    INPUT_SAVE = Style.BRIGHT + " Would you like to save the message? [Message: {}]" +\
        Fore.GREEN + " (Yes\\No) " + Style.RESET_ALL
    FORCED_HELP_LIMIT = 5

    def __init__(self, user, logger, database, commands):
        self.input_state = True
        self.user = user
        self.logger = logger
        self.database = database
        self.commands = commands
        self.prefix = Input.INPUT_PREFIX.format(user.role, Fore.GREEN + user.username + Fore.WHITE)

    def command_input(self):
        while self.input_state:
            self.next_command()

    def set_state(self, state):
        self.input_state = state

    def next_command(self):
        user_input = input(self.prefix)
        option = ""
        try:
            valid_pattern = r'^(\s*[a-zA-Z0-9_]+\s*)+(-[a-zA-Z0-9_])?\s*$'
            if not re.match(valid_pattern, user_input):
                raise ValueError(f"Not a valid character typed. "
                                 f"(you may type letters\\digits\\underscores and '-' at the beggining of the option)")
            args = re.findall(r'-?\b\w+\b', user_input)
            if len(args) == 0:
                raise ValueError(f"No command typed.")
            command = self.commands.get(args[0])
            if command is None:
                raise ValueError(f"'{args[0]}' is not a valid command.")
            else:
                args = args[1:]
                if len(args) > 0 and args[-1][0] == '-':
                    if command.is_opts:
                        option = args[-1]
                        args = args[-1:]
                    else:
                        raise ValueError(f"No options available for the command '{str(command)}'.")
                if len(args) > command.get_max_args():
                    raise TypeError(f"Too many arguments for the command '{str(command)}'.")
                elif len(args) < command.get_min_args():
                    raise TypeError(f"Too few arguments for the command '{str(command)}'.")
                command.execute(self.user, self.logger, args, option)

        except ValueError as e:
            self.logger.program_log(str(e), "error", 0)
        except TypeError as e:
            self.logger.program_log(str(e), "error", 0)

    def save_msg_input(self, message):
        flag = True
        while flag:
            clear_buffer()
            user_input = input(Input.INPUT_SAVE.format(message))
            if user_input.lower() in ['y', 'yes']:
                timestamp = datetime.now().strftime("%d-%m-%Y %H:%M:%S.%f")
                self.database.add_message(self.user.username, message, timestamp)
                self.logger.program_log("Message saved", "notice", 0)
                flag = False
            elif user_input.lower() in ['n', 'no']:
                self.logger.program_log("Message discarded", "notice", 0)
                flag = False
            else:
                self.logger.program_log("{} is not a valid input.".format(user_input), "error", 0)


class Command:

    def __init__(self, name, description, arg, func, func_args, opts, access):
        self.name = name
        self.description = description
        self.arg = arg
        self.func = func
        self.func_args = func_args
        self.opts = opts
        self.access = access

    def __str__(self):
        return self.name

    def opt_str(self):
        if len(self.opts) == 0:
            return ""
        str = "[" + self.opts[0][0] + "]"
        for option in self.opts[1:]:
            str += ", " + "[" +option[0] + "]"
        return str

    def arg_str(self):
        if self.arg[0] == "":
            return ""
        return "[" + self.arg[0] + "]"

    @property
    def is_opts(self):
        return len(self.opts) > 0

    def get_min_args(self):
        return self.arg[1]

    def get_max_args(self):
        return self.arg[2]

    def add_func_args(self, args):
        self.func_args.append(args)

    def execute(self, user, logger, args, option):
        if user.get_access() >= self.access:
            execution_args = list(self.func_args)
            if self.get_min_args() <= len(args) <= self.get_max_args():
                if len(args) > 0:
                    execution_args.append(args)
            else:
                raise TypeError(f"The command '{self.name}' must have between {self.get_min_args()}"
                                f" and {self.get_max_args()} arguments.")

            if option != "":
                option_func = self.opts.get(option)
                if option_func is None:
                    raise ValueError(f"'{option}' is not a valid option for '{self.name}'.")
                execution_args.append(option)

            self.func(*execution_args)
        else:
            logger.program_log("Access denied", "error", 0)

