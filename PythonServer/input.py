from colorama import Fore, Style, Back
from datetime import datetime

class Input:

    INPUT_PREFIX = Style.BRIGHT + " (--Covert_Channel--@{}<{}>) >> " + Style.RESET_ALL
    INPUT_SAVE = Style.BRIGHT + " Would you like to save the message? [Message: {}]" +\
        Fore.GREEN + " (Yes\\No) " + Style.RESET_ALL

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
        try:
            command = self.commands.get(user_input)
            if command is None:
                raise KeyError(f"'{user_input}' is not a valid command.")
            else:
                command[0](*command[1])
        except KeyError as e:
            self.logger.program_log(str(e), "error", 0)

    def save_msg_input(self, message):
        flag = True
        while flag:
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
