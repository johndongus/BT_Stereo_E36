# main.py

from UI import UIManager
from BT import BluetoothManager
import os
import threading
import logging
import signal
import sys

# Configure logging (ensure this is only done once)
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

def main():
    os.system("pulseaudio --start")
    logging.info("PulseAudio started.")
    
    ui_manager = UIManager()
    bt_manager = BluetoothManager(ui_manager=ui_manager)

    
    ui_manager.bt_manager = bt_manager

    

 
    ui_thread = threading.Thread(target=ui_manager.run, name="UIThread")
    ui_thread.start()

    

    bt_thread = threading.Thread(target=bt_manager.run, name="BluetoothThread")
    bt_thread.start()

    

    def signal_handler(sig, frame):
        logging.info("Shutdown signal received. Exiting gracefully...")
        bt_manager.running = False
        ui_manager.running = False
        bt_thread.join()
        ui_thread.join()
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Keep the main thread alive
    bt_thread.join()
    ui_thread.join()

if __name__ == "__main__":
    main()
