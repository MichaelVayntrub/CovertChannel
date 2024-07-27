from prettytable import PrettyTable, DOUBLE_BORDER
from colorama import Fore, Style, Back
from datetime import datetime
from utility import clear_buffer
import re


class Input:

    INPUT_PREFIX = Style.BRIGHT + "\n (--Covert_Channel--@{}<{}>) >> " + Style.RESET_ALL
    INPUT_SAVE = Style.BRIGHT + "Would you like to save the message? [Message: {}]" +\
        Fore.GREEN + " (Yes\\No) " + Style.RESET_ALL
    FORCED_HELP_LIMIT = 5

    def __init__(self, session, commands):
        self.input_state = True
        self.session = session
        self.commands = commands

    def command_input(self):
        while self.input_state:
            self.next_command()

    def set_state(self, state):
        self.input_state = state

    def get_char(): #test
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

    def next_command(self):
        user_input = input(Input.INPUT_PREFIX.format
                          (self.session.user.role, Fore.GREEN + self.session.user.username + Fore.WHITE))
        option = ""
        try:
            valid_pattern = r'^(\s*[a-zA-Z0-9_]+\s*)+([a-zA-Z0-9_])?\s*$'
            if not re.match(valid_pattern, user_input):
                raise ValueError(f"Not a valid character typed. "
                                 f"(you may type letters\\digits\\underscores)")
            args = re.findall(r'-?\b\w+\b', user_input)
            if len(args) == 0:
                raise ValueError(f"No command typed.")
            command = self.commands.get(args[0])
            if command is None:
                raise ValueError(f"'{args[0]}' is not a valid command.")
            else:
                args = args[1:]

                if len(args) > 1:
                    raise TypeError(f"Too many arguments for the command '{str(command)}'.")
                elif len(args) == 1:
                    option = args[0]
                else:
                    option = ""
                command.execute(self.session, option)

        except ValueError as e:
            self.session.logger.program_log(str(e), "error")
        except TypeError as e:
            self.session.logger.program_log(str(e), "error")

    def save_msg_input(self, time, message):
        flag = True
        while flag:
            clear_buffer()
            usage = self.session.network.calc_usage_percent()
            time = float(f"{time:.5f}")
            self.session.logger.program_log(f"Elapsed time: {time}", "info")
            self.session.logger.program_log(usage, "info")
            user_input = input(Input.INPUT_SAVE.format(message))
            if user_input.lower() in ['y', 'yes']:
                timestamp = datetime.now().strftime("%d-%m-%Y %H:%M:%S.%f")
                self.session.db.add_message(self.session.user.get_name(), message, timestamp)
                self.session.logger.program_log("Message saved", "notice")
                flag = False
            elif user_input.lower() in ['n', 'no']:
                self.session.logger.program_log("Message discarded", "notice")
                flag = False
            else:
                self.session.logger.program_log("{} is not a valid input.".format(user_input), "error")


class Command:

    def __init__(self, name, description, opts, access):
        self.name = name
        self.description = description
        self.access = access
        self.func = self.__str__
        self.opts = opts
        self.args = []

    def __str__(self):
        return self.name

    def opts_str(self):
        res = ""
        if len(self.opts) == 0:
            return res
        for option in self.opts:
            res += "[" + option + "] "
        return res

    def bind_func(self, func):
        self.func = func

    def add_func_options(self, opt):
        self.opts.append(opt)

    def execute(self, session, option):
        temp = list(self.args)
        if session.user.get_access() >= self.access:
            if option != "":
                if option not in self.opts:
                    raise ValueError(f"'{option}' is not a valid option for '{self.name}'.")
                temp.append(option)
            self.func(*temp)
        else:
            session.logger.program_log("Access denied", "error")

