import socket
import threading
import keyboard
import time
import os
from network import Network
from decipher import Decipher
from logger import Logger
from input import Input, Command
from database import Database
from user import User

def on_key_press(event):
    if event.name == 'esc':
        button_event.set()
        log = "Closing the servers..."
        program_logger.program_log(log, "notice", 0)
        end_run()
    elif event.name == 'tab':
        button_event.set()
        log = "Sent host's ip to the vm..."
        program_logger.program_log(log, "notice", 0)
        program_network.send_ip_to_vm()

def set_state(state):
    global current_state, state_mutex

    current_state = state
    if state == "input":
        program_input.set_state(True)
        program_logger.set_state(False)
    elif state == "running":
        program_input.set_state(False)
        program_logger.set_state(True)
    elif state == "exit":
        program_input.set_state(False)
        program_logger.set_state(False)
    state_mutex.release()

def state_input():
    if program_decipher.is_message:
        program_input.save_msg_input(program_decipher.message)
        program_decipher.clear_message()
    program_input.command_input()

def state_running():
    program_logger.program_log("\nRunning covert channel...", "notice", 0)
    for server in program_network.servers:
        thread = threading.Thread(target=server.run_server)
        threads.append(thread)
        thread.start()

def state_exit():
    global program_on
    program_logger.program_log("Closing the program...", "notice", 0)
    program_logger.program_log("Goodbye {}!".format(current_user.username), "notice", 0)
    for server in program_network.servers:
        if server.running: server.stop_server()
    for thread in threads:
        thread.join()
    keyboard.unhook_all()
    program_database.close_database()
    program_on = False

def end_run():
    for server in program_network.servers:
        if server.running: server.stop_server()
    for thread in threads:
        thread.join()
    set_state("input")

def run_program():
    global state_mutex, program_on, current_state

    while program_on:
        state_mutex.acquire()
        os.system("cls")
        program_logger.print_intro(current_state)
        states[current_state]()

def exit_program():
    set_state("exit")


# Program's variables initialization
program_logger = Logger()
program_database = Database(program_logger)
current_user = User("admin", "helicam")  # temp
#current_user = User("user", "anonimouse")  # temp
program_decipher = Decipher(program_logger)
program_network = Network(program_logger, program_decipher)

commands = {
    "run":      Command("run",
                "Starts a run of the covert channel",
                ("", 0, 0),
                set_state,
                ["running"],
                [],
                0
    ),
    "help":     Command("help",
                "Shows all the available commands",
                ("command\\none", 0, 1),
                program_logger.print_help,
                [current_user],
                [],
                0
    ),
    "exit":     Command("exit",
                "Exits the program",
                ("", 0, 0),
                exit_program,
                [],
                [],
                0
    ),
    "ip":      Command("ip",
                "Shows network's ips",
                ("", 0, 0),
                program_network.print_ip,
                [],
                [],
                1
    ),# temp
    "port":     Command("port",
                "Shows network's ports",
                ("", 0, 0),
                program_network.print_port,
                [],
                [],
                1
    ),# temp
    "msg":      Command("msg",
                "Shows all the messages",
                ("", 0, 0),
                program_database.fetch_table,
                ["messages"],
                [],
                1
    ),# temp
    "rain":     Command("rain",
                "Have you ever seen the rain?",
                ("", 0, 0),
                program_logger.matrix_rain,
                [],
                [],
                2
    ),
    "clear":    Command("clear",
                "Clears the terminal",
                ("", 0, 0),
                os.system,
                ["cls"],
                [],
                0
    )
}
states = {
    "running": state_running,
    "input": state_input,
    "exit": state_exit
}
program_input = Input(current_user, program_logger, program_database, commands)
program_input.commands.get("help").add_func_args(commands)
current_state = "input"
program_on = True
state_mutex = threading.Lock()
threads = []

# Key pressing event initialization
button_event = threading.Event()
event_thread = threading.Thread(target=button_event.wait, daemon=True)
event_thread.start()
keyboard.on_press(on_key_press)

run_program()
