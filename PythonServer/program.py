import socket
import threading
import keyboard
import time
import sys
import os
from network import Network
from decipher import Decipher
from logger import Logger
from input import Input
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
    state_mutex.release()

def state_input():
    if program_decipher.is_message:
        program_input.save_msg_input(program_decipher.message)
        program_decipher.clear_message()
    program_input.command_input()

def state_running():
    program_logger.program_log("Running covert channel...", "notice", 0)
    for server in program_network.servers:
        thread = threading.Thread(target=server.run_server)
        threads.append(thread)
        thread.start()

def end_run():
    for server in program_network.servers:
        if server.running: server.stop_server()
    for thread in threads:
        thread.join()
    set_state("input")

def run_program():
    global state_mutex

    while program_on:
        state_mutex.acquire()
        os.system("cls")
        program_logger.print_intro(current_state)
        states[current_state]()

def close_program():
    for server in program_network.servers:
        if server.running: server.stop_server()
    for thread in threads:
        thread.join()
    event_thread.join()

def exit_program():
    sys.exit()

# Program's variables initialization
program_logger = Logger()
program_database = Database(program_logger)
current_user = User("admin", "helicam")  # temp
program_decipher = Decipher(program_logger)
program_network = Network(program_logger, program_decipher)
commands = {
    "-run": (set_state, ["running"]),
    "-help": (program_logger.print_help, []),
    "-ip": (program_network.print_ip, []),
    "-port": (program_network.print_port, []),
    "-msg": (program_database.fetch_all, ["messages"]),
    "-rain": (program_logger.matrix_rain, []),
    "-exit": (exit_program, []),

    "-test": (program_database.drop_table, ["messages"]) # test
}
states = {
    "running": state_running,
    "input": state_input
}
program_input = Input(current_user, program_logger, program_database, commands)
current_state = "input"
program_on = True
state_mutex = threading.Lock()
threads = []

# Key pressing event initialization
button_event = threading.Event()
event_thread = threading.Thread(target=button_event.wait)
event_thread.start()
keyboard.on_press(on_key_press)

run_program()
