import colorama
from colorama import just_fix_windows_console
from colorama import Fore, Style, Back
from datetime import datetime
import threading
import time
import os

class Logger:

    PROGRAM_LOG_LENGTH = 100

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
        self.commands_desc = {
            "run": "",
            "help": "",
            "show": ( "", {
                "ip": "",
                "port": "",
                "msg": ""
            }),
            "exit": ""
        }
        self.running = False

    def set_state(self, state):
        self.running = state

    def print_intro_line(self, line, line_offset):
        intro_start = Fore.MAGENTA + "*" * 3 + " " * 3 + line
        intro_end = "*" * 3 + Style.RESET_ALL
        offset = self.offsets["Fore.MAGENTA"] + self.offsets["Style.RESET_ALL"] + line_offset
        print(intro_start + " " * (Logger.PROGRAM_LOG_LENGTH - len(intro_start) - len(intro_end) + offset) + intro_end)

    def command_format(self, command):
        offset = self.offsets["Fore.MAGENTA"] + self.offsets["Fore.GREEN"]
        return Fore.GREEN + "-" + command + Fore.MAGENTA, offset

    def print_intro(self, state):
        print(Fore.MAGENTA + "*" * Logger.PROGRAM_LOG_LENGTH + Style.RESET_ALL)
        self.print_intro_line("Python (Covert) Server v1.0", 0)
        if state == 'input':
            command1, offset1 = self.command_format('run')
            command2, offset2 = self.command_format('help')
            #command3, offset3 = self.command_format('exit')
            self.print_intro_line("Type {} to activate the servers or {} to view all the commands"
                                  .format(command1, command2), offset1 + offset2)
        elif state == 'running':
            self.print_intro_line("Press TAB to send host's ip to the vm", 0)
            self.print_intro_line("Press ESC to stop the covert run", 0)
        print(Fore.MAGENTA + "*" * Logger.PROGRAM_LOG_LENGTH + Style.RESET_ALL)
        print()

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

    def print_help(self):
        print("help yourself")

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
        return sum(self.color_offsets[color] for color in self.color_offsets.keys() if color in log)

    def program_print(self, log):
        with self.log_lock:
            if self.running:
                print("\x1b[F", end='', flush=True)
                print(log)
                self.print_covert_message()
            else:
                print(log)
