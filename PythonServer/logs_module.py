import colorama
from colorama import just_fix_windows_console
from colorama import Fore, Style, Back
from prettytable import PrettyTable, DOUBLE_BORDER
from datetime import datetime
import threading
import time
import os
import re

offsets = {
    "Fore.MAGENTA": len(Fore.MAGENTA),
    "Fore.YELLOW": len(Fore.YELLOW),
    "Fore.GREEN": len(Fore.GREEN),
    "Fore.CYAN": len(Fore.CYAN),
    "Fore.RED": len(Fore.RED),
    "Fore.LIGHTMAGENTA_EX": len(Fore.LIGHTMAGENTA_EX),
    "Fore.LIGHTBLUE_EX": len(Fore.LIGHTBLUE_EX),
    "Fore.LIGHTRED_EX": len(Fore.LIGHTRED_EX),
    "Style.RESET_ALL": len(Style.RESET_ALL)
}
color_offsets = {
    Fore.MAGENTA: len(Fore.MAGENTA),
    Fore.YELLOW: len(Fore.YELLOW),
    Fore.GREEN: len(Fore.GREEN),
    Fore.CYAN: len(Fore.CYAN),
    Fore.RED: len(Fore.RED),
    Fore.LIGHTMAGENTA_EX: len(Fore.LIGHTMAGENTA_EX),
    Fore.LIGHTBLUE_EX: len(Fore.LIGHTBLUE_EX),
    Fore.LIGHTRED_EX: len(Fore.LIGHTRED_EX),
    Style.RESET_ALL: len(Style.RESET_ALL)
}

def update_terminal(new_size):
    Logger.PROGRAM_LOG_LENGTH = new_size


class Logger:
    PROGRAM_LOG_LENGTH_MIN = 120
    PROGRAM_LOG_LENGTH_MAX = 170
    PROGRAM_LOG_LENGTH = 120
    PROTO_TEXT = {
        "tcp": "message received: ",
        "http": "requested path: "
    }

    def __init__(self):
        self.secret = "none"
        self.log_lock = threading.Lock()
        just_fix_windows_console()
        self.running = False

    def set_state(self, state):
        self.running = state

    #---intro-----------------------------------------------------------------------------------------------------------
    def print_intro(self, state = "input"):
        self.print_divider(Fore.MAGENTA)
        self.print_intro_line("Python (Covert) Server v2.0")
        if state == 'input':
            command1 = self.command_format('run')
            command2 = self.command_format('help')
            self.print_intro_line("Type {} to activate the servers or {} to view all the commands"
                                  .format(command1, command2))
        elif state == 'running':
            self.print_intro_line("Press ESC to stop the covert run")
        self.print_divider(Fore.MAGENTA)

    def print_intro_line(self, line):
        intro_start = Fore.MAGENTA + "*" * 3 + " " * 3 + line
        intro_end = "*" * 3 + Style.RESET_ALL
        offset = self.calc_offset(intro_start + intro_end)
        print(intro_start + " " * (Logger.PROGRAM_LOG_LENGTH - len(intro_start) - len(intro_end) + offset) + intro_end)

    def print_divider(self, color=Fore.WHITE):
        print(color + "*" * Logger.PROGRAM_LOG_LENGTH + Style.RESET_ALL)

    #---offset----------------------------------------------------------------------------------------------------------
    def calc_offset(self, log):
        log_updated = log
        sum = 0
        for color in color_offsets.keys():
            sum += log_updated.count(color) * color_offsets[color]
            log_updated = log_updated.replace(color, "", 1)
        return sum

    #---logs------------------------------------------------------------------------------------------------------------
    def program_print(self, log):
        with self.log_lock:
            if self.running:
                print("\x1b[F", end='', flush=True)
                print(log)
                self.print_covert_message()
            else:
                print(log)

    def program_log(self, log, type):
        types = {
            "connect": Fore.YELLOW,
            "notice": Fore.GREEN,
            "error": Fore.RED,
            "info": Style.BRIGHT
        }
        offset = self.calc_offset(log)
        self.program_print(types[type] + log + " " * (Logger.PROGRAM_LOG_LENGTH - len(log) + offset) + Style.RESET_ALL)

    def server_log(self, message, type, server_name):
        types = {
            "notice": [Fore.GREEN, "Fore.GREEN"],
            "establish": [Fore.CYAN, "Fore.CYAN"],
            "connect": [Fore.YELLOW, "Fore.YELLOW"],
            "error": [Fore.RED, "Fore.RED"]
        }
        log = "{}{} -- {}{}".format(self.server_intro(server_name), types[type][0], message, Style.RESET_ALL)
        log = self.pad_server_log(log, self.server_timestamp())
        self.program_print(log)

    #---format----------------------------------------------------------------------------------------------------------
    def command_format(self, command):
        return Fore.GREEN + command + Fore.MAGENTA

    def request_format(self, request):
       return Style.BRIGHT + request + Style.RESET_ALL

    def help_format(self):
        return Style.BRIGHT + " Usage: command [option]" + Style.RESET_ALL

    def help_command_format(self, command):
        pad = " " * (10 - len(str(command)))
        return Fore.LIGHTYELLOW_EX + str(command) + pad + Style.RESET_ALL

    #---covert----------------------------------------------------------------------------------------------------------
    def update_covert_message(self, secret):
        if secret == '': secret = "none"
        self.secret = secret

    def print_covert_message(self):
        msg_covert = "{Covert: " + self.secret + "}"
        padding_l = (int)((Logger.PROGRAM_LOG_LENGTH - len(msg_covert)) // 2)
        padding_r = Logger.PROGRAM_LOG_LENGTH - len(msg_covert) - padding_l
        msg_covert = Fore.LIGHTRED_EX + padding_l * "-" + msg_covert + padding_r * "-" + Style.RESET_ALL
        print(msg_covert)

    def matrix_rain(self):
        os.system("pymatrix-rain")

    #---server----------------------------------------------------------------------------------------------------------
    def pad_server_log(self, log, timestamp):
        char = '-'
        pad_offset = self.calc_offset(log)
        num_duplicates = Logger.PROGRAM_LOG_LENGTH - len(log) - len(timestamp) + pad_offset - 2
        padding = char * num_duplicates
        return log + " " + Fore.LIGHTMAGENTA_EX  + padding + " " + timestamp

    def server_timestamp(self):
        return "[" +  datetime.now().strftime('%d-%m-%Y %H:%M:%S.%f')[:-3] + "]"

    def server_intro(self, server_name):
        return Fore.LIGHTBLUE_EX + "Server {}:".format(server_name)

    def server_message(self, message, server_name, proto="tcp"):
        log = self.server_intro(server_name) + Fore.YELLOW + " -- " + Logger.PROTO_TEXT[proto] + Style.RESET_ALL + message
        log = self.pad_server_log(log, self.server_timestamp())
        self.program_print(log)
