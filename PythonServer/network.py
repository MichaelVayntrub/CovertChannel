import socket
import threading

class Server:

    def __init__(self, name, port, host, logger, decipher, packet_lock):
        self.name = name
        self.port = port
        self.host = host
        self.listening = False
        self.running = False
        self.logger = logger
        self.decipher = decipher
        self.packet_lock = packet_lock

    def run_server(self):
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind((self.host, self.port))
        server_socket.listen(1)
        server_socket.settimeout(1)
        self.running = True
        self.logger.server_log("listening on {}:{}".format(self.host, self.port), "establish", self.name)
        self.listening = True

        while self.running:
            try:
                connection, client_address = server_socket.accept()
                self.logger.server_log("connection from {}".format(client_address), "connect", self.name)

                while self.listening:
                    data = connection.recv(1024)
                    if data:
                        message = data.decode('utf-8')
                        with self.packet_lock:
                            self.logger.server_message(message, self.name)
                            self.decipher.receive_packet(self.name)
                connection.close()
            except socket.timeout:
                pass
        self.logger.server_log("server closed", "notice", self.name)

    def reset_connection(self):
        self.listening = False
        self.logger.server_log("connection reset", "connect", self.name)

    def stop_server(self):
        self.running = False
        self.listening = False


class Network:
    def __init__(self, logger, decipher):
        self.HOST_IP = self.set_current_ip()
        self.vm_ip = "192.168.56.1"
        self.PORT_A = 54321
        self.PORT_B = 54322
        self.vm_UDP_port = 54323
        self.packet_lock = threading.Lock()
        self.servers = [
            Server("A", self.PORT_A, self.HOST_IP, logger, decipher, self.packet_lock),
            Server("B", self.PORT_B, self.HOST_IP, logger, decipher, self.packet_lock)
        ]
        self.logger = logger
        self.decipher = decipher

    def print_ip(self):
        self.logger.print_ip(self.HOST_IP, self.vm_ip)

    def print_port(self):
        self.logger.print_port(self.PORT_A, self.PORT_B)

    def set_current_ip(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            s.connect(("8.8.8.8", 80))
            ip_address = s.getsockname()[0]
        finally:
            s.close()
        return ip_address

    def send_ip_to_vm(self):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.sendto(self.HOST_IP.encode(), (self.vm_ip, self.vm_UDP_port))