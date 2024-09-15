from utility import print_title, print_table
from logs_module import Logger, update_terminal
from input_module import Input, Command
from db_module import Database
from crypt_module import Decipher
from network_module import Network
from user import User, input_username_check
import threading
import keyboard
import time
import os

pressed_key = [None]

def main() -> None:
    # Global Variables initialization
    global program_input, session, state_mutex, program_on, current_state, threads
    current_state = "input"
    program_on = True
    state_mutex = threading.Lock()
    session = Session()
    session.bind_commands_func()
    session.add_args()
    update_terminal(session.db.get_terminal_size())
    program_input = Input(session, commands)
    threads = []

    # Key pressing event initialization
    global button_event, event_thread
    button_event = threading.Event()
    event_thread = threading.Thread(target=button_event.wait, daemon=True)
    event_thread.start()
    keyboard.on_press(on_key_press)

    run_program()

def run_program():
    global state_mutex, program_on, current_state

    while program_on:
        state_mutex.acquire()
        os.system("cls")
        session.logger.print_intro(current_state)
        states[current_state]()

def end_run():
    global pressed_key

    for server in session.network.servers:
        if server.running:
            server.stop_server()
    for thread in threads:
        thread.join()
    pressed_key[0] = 'none'
    set_state("input")

def set_state(state, protocol="tcp"):
    global current_state, state_mutex
    current_state = state

    if state == "input":
        program_input.set_state(True)
        session.logger.set_state(False)
    elif state == "running":
        if session.protocol != protocol:
            session.protocol = protocol
        program_input.set_state(False)
        session.logger.set_state(True)
    elif state == "exit":
        program_input.set_state(False)
        session.logger.set_state(False)
    state_mutex.release()

def on_key_press(event):
    global pressed_key, current_state

    if event.name == 'esc':
        if current_state == 'running':
            pressed_key[0] = event.name
            button_event.set()
            log = "Closing the servers..."
            session.logger.program_log(log, "notice")
            end_run()
    elif event.name == 'tab':
        if current_state == 'running':
            pressed_key[0] = event.name
            button_event.set()
            session.network.send_udp_vm("GO")
            session.network.timer = time.time()

#---states--------------------------------------------------------------------------------------------------------------
def state_input():
    if session.decipher.is_message:
        program_input.save_msg_input(session.network.timer, session.decipher.message)
        session.logger.secret = "none"
        session.decipher.clear_message()
    program_input.command_input()

def state_running():
    global pressed_key

    session.logger.program_log(f"\nRunning {session.protocol} covert channel...", "notice")
    if session.network.connect_to_vm():
        pressed_key[0] = 'none'
        for server in session.network.servers:
            if server.type == session.protocol:
                thread = threading.Thread(target=server.run_server, args=(button_event, pressed_key))
                threads.append(thread)
                thread.start()
    else:
        time.sleep(3)
        end_run()

def state_exit():
    global program_on
    session.logger.program_log("Closing the program...", "notice")
    session.logger.program_log("Goodbye {}!".format(session.user.username), "notice")
    for server in session.network.servers:
        if server.running: server.stop_server()
    for thread in threads:
        thread.join()
    keyboard.unhook_all()
    session.db.close_database()
    program_on = False

#---commands------------------------------------------------------------------------------------------------------------
def show_command(args=[]):
    options = {
        "msg": "messages",
        "user": "users",
        "ip": "ips",
        "port": "ports",
        "conf": "conf_machine"
    }
    if len(args) > 0:
        session.db.fetch_table(session, options[args])
    else:
        session.db.fetch_all_tables(session)

def edit_command(args=[]):
    options = {
        "PORT_A": ["PORT_A", "ports"],
        "PORT_B": ["PORT_B", "ports"],
        "PORT_UDP": ["vm_UDP_port", "ports"],
        "vm_ip": ["vm_ip", "ips"]
    }
    if len(args) > 0:
        new_value = input(session.logger.request_format("Choose new value: "))
        session.db.edit_table(session, options[args][0], options[args][1], new_value)
    else:
        session.logger.program_log("You must choose what to edit", "error")

def manage_command(args=[]):
    if len(args) > 0:
        if args == "add":
            try:
                username, user = input_username_check(session)
                if not user:
                    session.db.add_user(username, "user", 1)
                else:
                    session.logger.program_log("Username already exist", "error")
                session.logger.program_log(f"User {username} added successfully!", "notice")
            except SyntaxError as e:
                session.logger.program_log(str(e), "error")
            except ValueError as e:
                session.logger.program_log(str(e), "error")

        if args == "remove":
            old_user = input(session.logger.request_format("Enter username to remove: "))
            user = session.db.find_user(old_user)
            if user:
                session.db.remove_user(session, old_user, user[2])
            else:
                session.logger.program_log("Username doesn't exist", "error")
        if args == "promote":
            old_user = input(session.logger.request_format("Enter username to promote: "))
            user = session.db.find_user(old_user)
            if user:
                session.db.promote_user(session, old_user)
            else:
                session.logger.program_log("Username doesn't exist", "error")
    else:
        session.logger.program_log("You must choose an option", "error")

def update_terminal_size():
    size = input(session.logger.request_format("Choose a new size for the terminal [120 to 170]: "))
    session.db.update_terminal_size(session, int(size))

def update_algorithm(args=[]):
    if len(args) > 0:
        session.db.update_algorithm(session, args)
    else:
        session.logger.program_log("You must choose algorithm from the options", "error")

def print_help():
    commands_values = commands.values()
    filtered_access = lambda command: session.user.get_access() >= command.access
    command_desc = lambda command: [
        session.logger.help_command_format(command),
        command.description,
        command.opts_str()
    ]
    print_title("Help")
    print(session.logger.help_format())
    field_names = ["Command", "Description", "Options"]
    command_rows = list(map(command_desc, filter(filtered_access, commands_values)))
    print_table(field_names, command_rows)

def exit_program():
    set_state("exit")

#---session-------------------------------------------------------------------------------------------------------------
class Session:
    def __init__(self):
        self.logger = Logger()
        self.db = Database()
        self.decipher = Decipher()
        self.network = Network(self)

        #self.user =  User("guest", "user")
        self.user = User("admin", "helicam") #temp

        self.is_logged = False
        self.protocol = "tcp"
        self.file = "input.txt"

    def bind_commands_func(self):
        commands.get("help").bind_func(print_help)
        commands.get("intro").bind_func(self.logger.print_intro)
        commands.get("run").bind_func(set_state)
        commands.get("rain").bind_func(self.logger.matrix_rain)
        commands.get("login").bind_func(self.user.login)
        commands.get("user").bind_func(manage_command)
        commands.get("show").bind_func(show_command)
        commands.get("edit").bind_func(edit_command)
        commands.get("tsize").bind_func(update_terminal_size)
        commands.get("algo").bind_func(update_algorithm)
        commands.get("resetdb").bind_func(self.db.reset_db)
        commands.get("clear").bind_func(os.system)
        commands.get("exit").bind_func(exit_program)

    def add_args(self):
        commands.get("run").args.append("running")
        commands.get("login").args.append(self)
        commands.get("resetdb").args.append(self)
        commands.get("clear").args.append("cls")

#---const-------------------------------------------------------------------------------------------------------------
commands = {
    "help":     Command("help",
                        "Shows all the available (for you) commands",
                        [],
                        0
    ),
    "intro":    Command("intro",
                        "Prints the intro",
                        [],
                        0
    ),
    "run":      Command("run",
                        "Starts a run of the covert channel",
                        ["tcp", "http"],
                        1
    ),
    "rain":     Command("rain",
                        "Have you ever seen the rain?",
                        [],
                        3
    ),
    "login":    Command("login",
                        "Guest can only do so much",
                        [],
                        0
    ),
    "user":     Command("user",
                        "Users managment",
                        ["add", "remove", "promote"],
                        2
    ),
    "show":     Command("show",
                        "Shows some information from the database",
                        ["msg", "user", "ip", "port", "conf"],
                        1
    ),
    "edit":     Command("edit",
                        "Edit feature from the given options",
                        ["PORT_A", "PORT_B", "PORT_UDP", "vm_ip"],
                        2
    ),
    "tsize":    Command("tsize",
                        "Changes terminal's size",
                        [],
                        2
    ),
    "algo":    Command("algo",
                        "Choose the algorithm that used by the machine",
                        ["DFX"], #"CMDFX"
                        2
    ),
    "resetdb":  Command("resetdb",
                        "Reset the database",
                        [],
                        3
    ),
    "clear":    Command("clear",
                        "Clears the terminal",
                        [],
                        0
    ),
    "exit":     Command("exit",
                        "Exits the program",
                        [],
                        0
    )
}

states = {
    "running": state_running,
    "input": state_input,
    "exit": state_exit
}

if __name__ == '__main__':
    main()
