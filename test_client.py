import socket; import struct; import time
def pack_string(s):
    return struct.pack('<I', len(s)) + s.encode('utf-8')
def send_packet(s, ptype, data=b''):
    pay = struct.pack('<H', ptype) + data
    s.sendall(struct.pack('<I', len(pay)) + pay)
def recv_packet(s):
    head = s.recv(4)
    if not head: return None, None
    length = struct.unpack('<I', head)[0]
    payload = s.recv(length)
    if len(payload) < 2: return None, None
    ptype = struct.unpack('<H', payload[:2])[0]
    return ptype, payload[2:]

s1 = socket.socket(); s1.connect(('127.0.0.1', 8080))
send_packet(s1, 100, pack_string("Mr Krab") + pack_string("Password123!"))
print("Mr Krab logged in:", recv_packet(s1))
print("Mr Krab contact list:", recv_packet(s1))

s2 = socket.socket(); s2.connect(('127.0.0.1', 8080))
send_packet(s2, 100, pack_string("Bob") + pack_string("Password123!"))
print("Bob logged in:", recv_packet(s2))
print("Bob contact list:", recv_packet(s2))

print("Mr Krab recvs:", recv_packet(s1))

