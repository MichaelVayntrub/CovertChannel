import colorama
from colorama import just_fix_windows_console
from colorama import Fore, Style, Back
from prettytable import PrettyTable, DOUBLE_BORDER
from datetime import datetime
import threading
import time
import os
import re

class Logger:

    PROGRAM_LOG_LENGTH = 120

    def __init__(self):
        self.secret = "none"
        self.log_lock = threading.Lock()
        just_fix_windows_console()
        self.offsets = {
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
        self.color_offsets = {
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
        self.running = False

    def set_state(self, state):
        self.running = state

    def print_center_table(self, table):
        table_width = len(table.get_string().splitlines()[0])
        padding = (int)(Logger.PROGRAM_LOG_LENGTH - table_width ) // 2
        partitioned_table = table.get_string().splitlines()
        for row in partitioned_table:
            print(" " * padding + row)

    def print_title(self, title, color=Style.BRIGHT):
        msg_title = "{ " + title + " }"
        padding_l = (int)((Logger.PROGRAM_LOG_LENGTH - len(msg_title)) // 2)
        padding_r = Logger.PROGRAM_LOG_LENGTH - len(msg_title) - padding_l
        msg_title = color + padding_l * "-" + msg_title + padding_r * "-" + Style.RESET_ALL
        print('\n' + msg_title)

    def print_divider(self, color=Fore.WHITE):
        print(color + "*" * Logger.PROGRAM_LOG_LENGTH + Style.RESET_ALL)

    def print_intro_line(self, line, line_offset):
        intro_start = Fore.MAGENTA + "*" * 3 + " " * 3 + line
        intro_end = "*" * 3 + Style.RESET_ALL
        offset = self.offsets["Fore.MAGENTA"] + self.offsets["Style.RESET_ALL"] + line_offset
        print(intro_start + " " * (Logger.PROGRAM_LOG_LENGTH - len(intro_start) - len(intro_end) + offset) + intro_end)

    def command_format(self, command):
        offset = self.offsets["Fore.MAGENTA"] + self.offsets["Fore.GREEN"]
        return Fore.GREEN + command + Fore.MAGENTA, offset

    def print_intro(self, state):
        self.print_divider(Fore.MAGENTA)
        self.print_intro_line("Python (Covert) Server v1.0", 0)
        if state == 'input':
            command1, offset1 = self.command_format('run')
            command2, offset2 = self.command_format('help')
            self.print_intro_line("Type {} to activate the servers or {} to view all the commands"
                                  .format(command1, command2), offset1 + offset2)
        elif state == 'running':
            self.print_intro_line("Press TAB to send host's ip to the vm", 0)
            self.print_intro_line("Press ESC to stop the covert run", 0)
        self.print_divider(Fore.MAGENTA)

    def update_covert_message(self, secret):
        if secret == '': secret = "none"
        self.secret = secret

    def print_covert_message(self):
        #self.print_title("Covert: " + self.secret, Fore.LIGHTRED_EX)
        msg_covert = "{Covert: " + self.secret + "}"
        padding_l = (int)((Logger.PROGRAM_LOG_LENGTH - len(msg_covert)) // 2)
        padding_r = Logger.PROGRAM_LOG_LENGTH - len(msg_covert) - padding_l
        msg_covert = Fore.LIGHTRED_EX + padding_l * "-" + msg_covert + padding_r * "-" + Style.RESET_ALL
        print(msg_covert)

    def matrix_rain(self):
        os.system("pymatrix-rain")

    def print_help(self, user, commands, args=[]):
        commands_values = commands.values()
        filtered_access = lambda command: user.get_access() >= command.access
        command_desc = lambda command: [
            Fore.LIGHTYELLOW_EX + str(command) + Style.RESET_ALL,
            command.description,
            command.arg_str(),
            command.opt_str()
        ]
        if len(args) == 0:
            self.print_title("Help")
            print(Style.BRIGHT + " Usage: command [arguments] [-option]" + Style.RESET_ALL)
            field_names = ["Command", "Description", "Argument", "Options"]
            command_rows = list(map(command_desc, filter(filtered_access, commands_values)))
            self.print_table(field_names, command_rows)
        else:
            specific_command = args[0]
            argument = commands.get(specific_command)
            if argument is None:
                raise ValueError(f"'{specific_command}' is not a valid command name.")
            else:
                print(specific_command)

    def print_ip(self, host_ip, vm_ip):
        print(Fore.YELLOW + "Host ip: " + Style.RESET_ALL + host_ip)
        print(Fore.YELLOW + "VM ip: " + Style.RESET_ALL + vm_ip)

    def print_port(self, port_a, port_b):
        print(Fore.YELLOW + "Port A: " + Style.RESET_ALL + str(port_a))
        print(Fore.YELLOW + "Port B: " + Style.RESET_ALL + str(port_b))

    def pad_server_log(self, log, timestamp, offset):
        char = '-'
        pad_offset = offset + self.offsets["Fore.LIGHTMAGENTA_EX"]
        num_duplicates = Logger.PROGRAM_LOG_LENGTH - len(log) - len(timestamp) + pad_offset - 2
        padding = char * num_duplicates
        return log + " " + Fore.LIGHTMAGENTA_EX  + padding + " " + timestamp

    def server_timestamp(self):
        return "[" +  datetime.now().strftime('%d-%m-%Y %H:%M:%S.%f')[:-3] + "]"

    def server_intro(self, server_name):
        return Fore.LIGHTBLUE_EX + "Server {}:".format(server_name)

    def server_message(self, message, server_name):
        log = self.server_intro(server_name) + Fore.YELLOW + " -- " + "message received: " + Style.RESET_ALL + message
        offset = self.offsets["Fore.YELLOW"] + self.offsets["Style.RESET_ALL"]
        log = self.pad_server_log(log, self.server_timestamp(), offset)
        self.program_print(log)

    def server_log(self, message, type, server_name):
        types = {
            "notice": [Fore.GREEN, "Fore.GREEN"],
            "establish": [Fore.CYAN, "Fore.CYAN"],
            "connect": [Fore.YELLOW, "Fore.YELLOW"],
            "error": [Fore.RED, "Fore.RED"]
        }
        offset = self.offsets[types[type][1]] + self.offsets["Style.RESET_ALL"]
        log = "{}{} -- {}{}".format(self.server_intro(server_name), types[type][0], message, Style.RESET_ALL)
        log = self.pad_server_log(log, self.server_timestamp(), offset)
        self.program_print(log)

    def program_log(self, log, type, offset):
        types = {
            "notice": Fore.GREEN,
            "error": Fore.RED
        }
        self.program_print(types[type] + log + " " * (Logger.PROGRAM_LOG_LENGTH - len(log) + offset) + Style.RESET_ALL)

    def calc_offset(self, log):
        log_updated = log
        sum = 0
        for color in self.color_offsets.keys():
            while color in log_updated:
                log_updated = log_updated.replace(color, "", 1)
                sum += self.color_offsets[color]
        return sum

    def program_print(self, log):
        with self.log_lock:
            if self.running:
                print("\x1b[F", end='', flush=True)
                print(log)
                self.print_covert_message()
            else:
                print(log)

    def print_table(self, fields, rows):
        brighten = lambda field: Style.BRIGHT + field + Style.RESET_ALL
        table = PrettyTable()
        table.field_names = list(map(brighten, fields))
        table.set_style(DOUBLE_BORDER)
        table.align = "l"
        table.padding_width = 3

        for row in rows:
            table.add_row(row)

        print()
        self.print_center_table(table)
        print()