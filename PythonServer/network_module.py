from utility import set_timer
import socket
import threading
import time

http_mapping = {
    "/": "html/index.html",
    "/a": "html/a.html",
    "/b": "html/b.html"
}

class Server:
    TIMEOUT = 5

    def __init__(self, name, session, port, host, packet_lock, type):
        self.name = name
        self.port = port
        self.host = host
        self.type = type
        self.listening = False
        self.running = False
        self.session = session
        self.packet_lock = packet_lock

    def run_server(self, stop_event, pressed_key):
        with self.packet_lock:
            self.session.network.active_servers += 1
        logger, decipher = self.session.logger, self.session.decipher
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((self.host, self.port))
        server_socket.listen(1)
        server_socket.settimeout(1)
        self.running = True
        self.session.logger.server_log("listening on {}:{}".format(self.host, self.port), "establish", self.name)
        self.listening = True

        while self.running:
            try:
                if self.session.protocol == "tcp":
                    connection, client_address = server_socket.accept()
                    logger.server_log("connection from {}".format(client_address), "connect", self.name)
                while self.listening:
                    if self.session.protocol == "tcp":
                        self.run_tcp_server(logger, decipher, connection)
                    elif self.session.protocol == "http":
                        connection, client_address = server_socket.accept()
                        self.run_http_server(logger, decipher, connection)
                    else:
                        raise Exception("protocol not supported")
            except socket.timeout:
                if stop_event.is_set() and pressed_key[0] == 'esc':
                    self.stop_server()
                    break
            except Exception as e:
                logger.server_log(str(e), "error", self.name)
        server_socket.close()
        logger.server_log("server closed", "notice", self.name)

    def run_http_server(self, logger, decipher, connection):
        request = connection.recv(1024).decode('utf-8')
        if request:
            with self.packet_lock:
                self.session.network.add_packet_usage(len(request))
            request_line = request.splitlines()[0]
            req_path = request_line.split()[1]

            if req_path in http_mapping.keys():
                with open(http_mapping[req_path], 'r') as file:
                    response_body = file.read()
                    if req_path in ["/a", "/b"]:
                        with self.packet_lock:
                            logger.server_message("'" + req_path + "'", self.name, "http")
                            decipher.receive_packet(req_path, logger)
                response_status = "HTTP/1.1 200 OK"
            else:
                response_body = "404 Not Found"
                response_status = "HTTP/1.1 404 Not Found"

            response = (
                f"{response_status}\r\n"
                "Content-Type: text/html\r\n"
                f"Content-Length: {len(response_body)}\r\n"
                "Connection: keep-alive\r\n"
                "\r\n"
                f"{response_body}"
            )
            connection.sendall(response.encode('utf-8'))
        else:
            connection.detach()
            connection.close()
            time.sleep(1)
            self.stop_server()

    def run_tcp_server(self, logger, decipher, connection):
        data = connection.recv(1024)
        if data:
            message = data.decode('utf-8')
            with self.packet_lock:
                self.session.network.add_packet_usage(len(data))
                logger.server_message(message, self.name, "tcp")
                decipher.receive_packet(self.name, logger)
        else:
            connection.detach()
            connection.close()
            time.sleep(1)
            self.stop_server()

    def reset_connection(self):
        self.listening = False
        self.session.logger.server_log("connection reset", "connect", self.name)

    def stop_server(self):
        with self.packet_lock:
            self.session.network.active_servers -= 1
        if self.session.network.active_servers == 0:
            end_time = time.time()
            self.session.network.timer = end_time - self.session.network.timer
        self.running = False
        self.listening = False


class Network:
    LOCAL_HOST = "127.0.0.1"
    TEST_IP = "8.8.8.8"
    HTTP_PORT = 80
    TIMEOUT = 5
    MAX_ATTEMPTS = 5

    def __init__(self, session):
        # Default settings
        self.vm_ip = "192.168.56.1"
        self.PORT_A = 54321
        self.PORT_B = 54322
        self.vm_UDP_port = 54323
        self.vm_res_port = 54324
        # Variables initialization
        self.session = session
        self.HOST_IP = self.set_current_ip()
        self.update_ips()
        self.update_ports()
        self.packet_lock = threading.Lock()
        self.timer = 0
        self.active_servers = 0
        self.packets_total_size = 0
        # Servers initialization
        self.servers = [
            Server("TCP_A", session, self.PORT_A, Network.LOCAL_HOST, self.packet_lock, "tcp"),
            Server("TCP_B", session, self.PORT_B, Network.LOCAL_HOST, self.packet_lock, "tcp"),
            Server("HTTP", session, Network.HTTP_PORT, Network.LOCAL_HOST, self.packet_lock, "http")
        ]

    def calc_usage_percent(self):
        message_size = len(self.session.decipher.message)
        usage = float(f"{message_size / self.packets_total_size:.5f}")
        return f"Message size = {message_size} bytes\n" \
               f"Packets size = {self.packets_total_size} bytes\n" \
               f"Usage percentage = {usage}%"

    def add_packet_usage(self, packet_size):
        self.packets_total_size += packet_size

    def update_ips(self):
        ips = self.session.db.get_ips()
        self.vm_ip = ips["vm_ip"]

    def update_ports(self):
        ports = self.session.db.get_ports()
        self.PORT_A = ports["PORT_A"]
        self.PORT_B = ports["PORT_B"]
        self.vm_UDP_port = ports["vm_UDP_port"]

    def print_ip(self):
        self.logger.print_ip(self.HOST_IP, self.vm_ip)

    def print_port(self):
        self.logger.print_port(self.PORT_A, self.PORT_B)

    def set_current_ip(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            s.connect((Network.TEST_IP, Network.HTTP_PORT))
            ip_address = s.getsockname()[0]
        finally:
            s.close()
        return ip_address

    def send_udp_vm(self, message):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.sendto(message.encode(), (self.vm_ip, self.vm_UDP_port))

    def connect_to_vm(self):
        message = self.create_conf_message()
        self.session.logger.program_log("Connecting to the vm...", "notice")
        return self.listen_to_vm_response(message)

    def listen_to_vm_response(self, message):
        attempt_count = 0
        while attempt_count < Network.MAX_ATTEMPTS:
            try:
                self.send_udp_vm(message)
                response_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                response_sock.bind((Network.LOCAL_HOST, self.vm_res_port))
                response_sock.settimeout(Network.TIMEOUT)
                while True:
                    data, addr = response_sock.recvfrom(1024)
                    data = data.decode('utf-8')
                    if data == "OK":
                        self.is_vm_available = True
                        response_sock.close()
                        self.session.logger.program_log("Connection established - Press TAB to start", "connect")
                        return True
            except socket.timeout:
                attempt_count += 1
        response_sock.close()
        self.session.logger.program_log("Connection failed - vm not available!", "error")
        return False


    def create_conf_message(self):
        message = self.HOST_IP + ';'
        message += self.session.protocol + ';'
        message += self.session.file + ';'
        message += self.session.decipher.mode
        return message
