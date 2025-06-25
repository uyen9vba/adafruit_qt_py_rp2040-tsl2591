import serial
from serial.tools import list_ports
import time
import json
import subprocess

def connect():
    while True:
        print("waiting for pid 4508 device")
        for port in list_ports.comports():
            hwid = port.hwid
            start = hwid.find('PID') + 4
            colon = hwid.find(':', start)
            space = hwid.find(' ', colon)
            pid = hwid[colon+1:space]
            if pid != "4508":
                continue
            con = serial.Serial(port.device, 115200, timeout=1)
            print("found pid 4508 device, opening serial port")
            return con
        time.sleep(1)

def task(con=None):
    buffer = ""
    ready = False
    while True:
        if con.in_waiting <= 0:
            time.sleep(0.1)
            continue

        data = con.read(con.in_waiting).decode('utf-8')
        buffer += data
        while '\n' in buffer:
            line, buffer = buffer.split('\n', 1)
            try:
                json_dict = json.loads(line)
                print(json_dict)
                lux = json_dict['lux']
                ready = True
            except json.JSONDecodeError:
                print(f"decode error, is this json?")

        if not ready:
            time.sleep(0.1)
            continue
       
        ready = False
        lux_min = 0
        lux_max = 500
        lux_range = lux_max - lux_min
        output_min = 0
        output_max = 80
        output_range = output_max - output_min
        output = (((lux - lux_min) * output_range) / lux_range) + output_min
        print(f"lux {lux} output {output}")
        #result = subprocess.run(['twinkletray', '--List'], capture_output=True, text=True)
        subprocess.run(['twinkletray', '--MonitorNum=2', f"--Set={output}"], shell=True)

        time.sleep(0.1)

if __name__ == "__main__":
    while True:
        try:
            con = connect()
            task(con)
        except Exception as e:
            print(e)
 
