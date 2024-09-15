import unittest
from unittest.mock import patch
from colorama import Fore, Style, Back
from input_module import Input, Command
from db_module import Database
from network_module import Network, Server
from crypt_module import Decipher
from logs_module import Logger
from user import User
from io import StringIO
import sys
import socket
import threading
import keyboard
from main import Session, on_key_press


class test_receive_packet(unittest.TestCase):
    def test_blank_tcp(self):
        logger = Logger()
        decipher = Decipher()
        channels = ["TCP_A", "TCP_B"]
        expected = "\0"
        for i in range(16):
            decipher.receive_packet(channels[i % 2], logger)
        self.assertEqual(decipher.message, expected)

    def test_blank_http(self):
        logger = Logger()
        decipher = Decipher()
        channels = ["/a", "/b"]
        expected = "\0"
        for i in range(16):
            decipher.receive_packet(channels[i % 2], logger)
        self.assertEqual(decipher.message, expected)

    def test_regular_packet_tcp(self):
        captured_output = StringIO()
        sys.stdout = captured_output
        logger = Logger()
        decipher = Decipher()
        channels = ["TCP_A", "TCP_B"]
        bits = [0,1,0,1,0,1,0,1,1,0,0,1,0,1,1,0]
        expected = "A"
        for i in range(16):
            decipher.receive_packet(channels[bits[i]], logger)
        self.assertEqual(decipher.message, expected)

    def test_regular_packet_http(self):
        captured_output = StringIO()
        sys.stdout = captured_output
        logger = Logger()
        decipher = Decipher()
        channels = ["/a", "/b"]
        bits = [0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0]
        expected = "A"
        for i in range(16):
            decipher.receive_packet(channels[bits[i]], logger)
        self.assertEqual(decipher.message, expected)


class test_covert_mirrored_xor(unittest.TestCase):
    def test_blank(self):
        blank = 0b01010101
        logger = Logger()
        decipher = Decipher()
        left = 0b01010101
        right = 0b01010101
        res = decipher.covert_mirrored_xor(left, right)
        self.assertEqual(res, "\0")

    def test_regular_packet(self):
        A = 0b01000001
        logger = Logger()
        decipher = Decipher()
        left = 0b01010101
        right = 0b10010110
        res = decipher.covert_mirrored_xor(left, right)
        self.assertEqual(res, "A")


class test_logger(unittest.TestCase):
    def test_print_ip(self):
        logger = Logger()
        test_ip = "1.1.1.1"
        captured_output = StringIO()
        sys.stdout = captured_output
        logger.print_ip(test_ip, test_ip)
        printed_output = captured_output.getvalue().strip()
        sys.stdout = sys.__stdout__
        res =  Fore.YELLOW + "Host ip: " + Style.RESET_ALL + test_ip
        res += '\n'
        res +=  Fore.YELLOW + "VM ip: " + Style.RESET_ALL + test_ip
        self.assertEqual(printed_output, res)

    def test_print_port(self):
        logger = Logger()
        test_port = 1111
        captured_output = StringIO()
        sys.stdout = captured_output
        logger.print_port(test_port, test_port)
        printed_output = captured_output.getvalue().strip()
        sys.stdout = sys.__stdout__
        res =  Fore.YELLOW + "Port A: " + Style.RESET_ALL + str(test_port)
        res += '\n'
        res +=  Fore.YELLOW + "Port B: " + Style.RESET_ALL + str(test_port)
        self.assertEqual(printed_output, res)

    def test_calc_offset(self):
        logger = Logger()
        expected_offset = Logger.color_offsets[Fore.YELLOW] + logger.color_offsets[Fore.YELLOW] \
                          + 2 * logger.color_offsets[Style.RESET_ALL]
        test_string = Fore.YELLOW + "This is " + Style.RESET_ALL
        test_string += Fore.YELLOW + "a test: " + Style.RESET_ALL
        result_offset = logger.calc_offset(test_string)
        self.assertEqual(result_offset, expected_offset)

class test_input(unittest.TestCase):
    @patch('sys.stdin', StringIO('test'))
    def test_next_command_read(self):
        captured_output = StringIO()
        sys.stdout = captured_output
        def test_func(a): a[0] += 7
        a = [7]
        commands = {
            "test": Command("test",
                   "This is a test",
                   #("", 0, 0),
                   #test_func,
                   #[a],
                   [],
                   0
            ),
        }
        session = Session();
        input = Input(session, commands)
        input.next_command()
        self.assertEqual(a[0], 14)

    @patch('sys.stdin', StringIO('test arg'))
    def test_access_denied(self):
        test_func = lambda a: a + 7
        logger = Logger()
        database = Database()
        user = User("user", "test_user")
        commands = {
            "test": Command("test",
                    "This is a test",
                    #("arg", 1, 1),
                    #test_func,
                    #[7],
                    [],
                    1
            ),
        }
        session = Session();
        input = Input(session, commands)

        expected = "Access denied"
        expected = Fore.RED + expected + " " * (Logger.PROGRAM_LOG_LENGTH - len(expected)) + Style.RESET_ALL
        expected = input.prefix + expected

        captured_output = StringIO()
        sys.stdout = captured_output
        input.next_command()
        printed_output = captured_output.getvalue().strip()
        sys.stdout = sys.__stdout__
        self.assertEqual(printed_output, expected)

    @patch('sys.stdin', StringIO('@test'))
    def test_next_wrong_char(self):
        test_func = lambda a: a + 7
        logger = Logger()
        database = Database()
        user = User("user", "test_user")
        commands = {
            "test": Command("test",
                    "This is a test",
                    #("", 0, 0),
                    #test_func,
                    #[7],
                    [],
                    0
                    ),
        }
        session = Session();
        input = Input(session, commands)

        expected = "Not a valid character typed. (you may type letters\\digits\\underscores and '-' " \
                   "at the beggining of the option)"
        expected = Fore.RED + expected + " " * (Logger.PROGRAM_LOG_LENGTH - len(expected)) + Style.RESET_ALL
        expected = input.prefix + expected

        captured_output = StringIO()
        sys.stdout = captured_output
        input.next_command()
        printed_output = captured_output.getvalue().strip()
        sys.stdout = sys.__stdout__
        self.assertEqual(printed_output, expected)

    @patch('sys.stdin', StringIO('test arg1 arg2'))
    def test_next_too_many_arguments(self):
        test_func = lambda a: a + 7
        logger = Logger()
        database = Database(logger)
        user = User("user", "test_user")
        commands = {
            "test": Command("test",
                    "This is a test",
                    ["arg"],
                    0
                    ),
        }
        session = Session();
        input = Input(session, commands)

        expected = "Too many arguments for the command '{}'.".format("test")
        expected = Fore.RED + expected + " " * (Logger.PROGRAM_LOG_LENGTH - len(expected)) + Style.RESET_ALL
        expected = input.prefix + expected

        captured_output = StringIO()
        sys.stdout = captured_output
        input.next_command()
        printed_output = captured_output.getvalue().strip()
        sys.stdout = sys.__stdout__
        self.assertEqual(printed_output, expected)


class test_server(unittest.TestCase):
    def test_listening_port(self):
        pressed_key = [None]
        pressed_key[0] = 'none'
        button_event = threading.Event()
        event_thread = threading.Thread(target=button_event.wait, daemon=True)
        event_thread.start()
        keyboard.on_press(on_key_press)

        session = Session()
        network = Network(session)
        test_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        res = [False, test_sock]

        def is_listen(port, res):
            res[1].settimeout(1)
            result = res[1].connect_ex((network.HOST_IP, port))
            if result == 0: res[0] = True
            else: res[0] = False

        threadA = threading.Thread(target=network.servers[0].run_server, args=(button_event, pressed_key))
        threadA.start()
        is_listen(network.PORT_A, res)
        self.assertTrue(res[0])
        res[1].close()
        network.servers[0].stop_server()
        threadA.join()