import asyncio
import serial
import time
import pyautogui
from comtypes import CLSCTX_ALL
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume
from ctypes import cast, POINTER
from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as Manager

# --- CONFIGURATION ---
PORT = 'COM3'  
BAUD = 9600

# 1. Initialize Serial ONCE
try:
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    print(f"Successfully connected to {PORT}")
except Exception as e:
    print(f"Serial Error: {e}. Ensure Arduino IDE Monitor is CLOSED.")
    exit()

# 2. Setup Audio Interface
def setup_audio():
    try:
        device_enumerator = AudioUtilities.GetDeviceEnumerator()
        devices = device_enumerator.GetDefaultAudioEndpoint(0, 1)
        interface = devices.Activate(IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
        return cast(interface, POINTER(IAudioEndpointVolume))
    except Exception as e:
        print(f"Audio Init Error: {e}")
        return None

def toggle_playback():
    pyautogui.press('playpause')
    print("Action: Toggle Play/Pause")

async def get_media_data():
    try:
        sessions = await Manager.request_async()
        current_session = sessions.get_current_session()
        if current_session:
            info = await current_session.try_get_media_properties_async()
            timeline = current_session.get_timeline_properties()
            if info:
                title = info.title[:20]  # Limit length for OLED
                artist = info.artist[:20]
                duration = timeline.end_time.total_seconds()
                pos = timeline.position.total_seconds()
                percent = int((pos / duration) * 100) if duration > 0 else 0
                return f"{title} - {artist}|{percent}"
    except:
        pass
    return "Nothing Playing|0"

async def main_loop():
    volume_interface = setup_audio()
    last_media_data = ""
    time.sleep(2) # Wait for Arduino reset

    print("Bridge Active. Listening for Knob, Button, and Media changes...")

    while True:
        # --- PHASE 1: ARDUINO -> PC (Read) ---
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8').strip()
                
                if line == "<TOGGLE>":
                    toggle_playback()
                
                elif line.isdigit():
                    vol_input = int(line)
                    normalized_vol = max(0.0, min(1.0, vol_input / 100.0))
                    if volume_interface:
                        volume_interface.SetMasterVolumeLevelScalar(normalized_vol, None)
                        print(f"Volume: {int(normalized_vol * 100)}%")
            except Exception as e:
                print(f"Read Error: {e}")

        # --- PHASE 2: PC -> ARDUINO (Write Media Info) ---
        try:
            current_media = await get_media_data()
            if current_media != last_media_data:
                # We send it in brackets < > so your Arduino can parse it easily
                ser.write(f"<{current_media}>".encode('utf-8'))
                last_media_data = current_media
        except Exception as e:
            print(f"Write Error: {e}")

        await asyncio.sleep(0.05) # Fast enough for volume, slow enough for CPU

if __name__ == "__main__":
    try:
        asyncio.run(main_loop())
    except KeyboardInterrupt:
        print("Stopping...")
        ser.close()