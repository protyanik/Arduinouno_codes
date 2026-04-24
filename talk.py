import asyncio
import serial
import time
from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as Manager

# --- IMPORTANT: Change 'COM3' to YOUR port number ---
PORT = 'COM3' 

try:
    ser = serial.Serial(PORT, 9600, timeout=1)
    print(f"Connected to Arduino on {PORT}")
except:
    print(f"Error: Could not open {PORT}. Is the Serial Monitor closed?")
    exit()

async def get_media_info():
    sessions = await Manager.request_async()
    current_session = sessions.get_current_session()
    if current_session:
        info = await current_session.try_get_media_properties_async()
        if info:
            return f"{info.title} - {info.artist}"
    return "Nothing Playing"

async def main():
    last_info = ""
    print("Searching for active media...")
    while True:
        try:
            info = await get_media_info()
            if info != last_info:
                # This sends the data to your Arduino
                ser.write(f"<{info}>".encode('utf-8'))
                print(f"Sent to OLED: {info}")
                last_info = info
        except Exception as e:
            print(f"Connection Error: {e}")
        
        await asyncio.sleep(2)

if __name__ == "__main__":
    asyncio.run(main())