from colorama import Fore, Style, Back
from prettytable import PrettyTable, DOUBLE_BORDER
from logs_module import Logger
import threading
import time
import msvcrt

def set_timer(time, timeout_func):
    timer = threading.Timer(time, timeout_func)
    timer.start()

def clear_buffer():
    while msvcrt.kbhit():
        msvcrt.getch()

#---tables--------------------------------------------------------------------------------------------------------------
def print_title(title, color=Style.BRIGHT):
    msg_title = "{ " + title + " }"
    padding_l = (int)((Logger.PROGRAM_LOG_LENGTH - len(msg_title)) // 2)
    padding_r = Logger.PROGRAM_LOG_LENGTH - len(msg_title) - padding_l
    msg_title = color + padding_l * "-" + msg_title + padding_r * "-" + Style.RESET_ALL
    print('\n' + msg_title)

def print_center_table(table):
    table_width = len(table.get_string().splitlines()[0])
    padding = (int)(Logger.PROGRAM_LOG_LENGTH - table_width ) // 2
    partitioned_table = table.get_string().splitlines()
    for row in partitioned_table:
        print(" " * padding + row)

def print_table(fields, rows, title=None):
    if title:
        print_title(title)

    brighten = lambda field: Style.BRIGHT + field + Style.RESET_ALL
    table = PrettyTable()
    table.field_names = list(map(brighten, fields))
    table.set_style(DOUBLE_BORDER)
    table.align = "c"
    table.padding_width = 4

    for row in rows:
        table.add_row(row)
    print()
    print_center_table(table)
    print()
