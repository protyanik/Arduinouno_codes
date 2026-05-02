import asyncio
import serial
import serial.tools.list_ports
import time
import pyautogui
from comtypes import CLSCTX_ALL
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume
from ctypes import cast, POINTER
from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as Manager

# --- CONFIGURATION ---
BAUD_RATE = 9600
# If you want to hardcode the port, change this to 'COM3' etc.
# Otherwise, it will try to find it automatically.
TARGET_PORT = None 

def find_arduino():
    """Attempts to find the Arduino port automatically."""
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if "Arduino" in p.description or "CH340" in p.description or "USB Serial" in p.description:
            return p.device
    return None

def setup_audio():
    """Initializes the Windows Core Audio API for volume control."""
    try:
        device_enumerator = AudioUtilities.GetDeviceEnumerator()
        devices = device_enumerator.GetDefaultAudioEndpoint(0, 1) # 0=Render, 1=Multimedia
        interface = devices.Activate(IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
        return cast(interface, POINTER(IAudioEndpointVolume))
    except Exception as e:
        print(f"[-] Audio Interface Error: {e}")
        return None

async def get_media_data():
    """Fetches current song, artist, and progress from Windows Media Session."""
    try:
        sessions = await Manager.request_async()
        current_session = sessions.get_current_session()
        
        if current_session:
            info = await current_session.try_get_media_properties_async()
            timeline = current_session.get_timeline_properties()
            
            if info:
                title = info.title[:25] # Truncate for OLED space
                artist = info.artist[:20]
                album = info.album_title[:20] if info.album_title else "Single"
                
                duration = timeline.end_time.total_seconds()
                pos = timeline.position.total_seconds()
                percent = int((pos / duration) * 100) if duration > 0 else 0
                
                # Format exactly as the Arduino parser expects
                return f"{title} - {artist}|{album}|{percent}"
    except Exception:
        pass # Ignore errors during session changes
    return "Nothing Playing|...|0"

async def bridge_loop():
    """Main execution loop handling bi-directional communication."""
    port = TARGET_PORT or find_arduino()
    if not port:
        print("[-] Error: No Arduino found. Please check your connection.")
        return

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=0.1)
        time.sleep(2) # Allow Arduino to reboot after serial connection
        print(f"[+] Connected to {port} at {BAUD_RATE} baud.")
    except Exception as e:
        print(f"[-] Serial Error: {e}")
        return

    volume_interface = setup_audio()
    last_media_data = ""

    print("[*] Bridge is active. Controls and OLED updates are running...")

    while True:
        # --- 1. HANDLE INCOMING (Arduino -> PC) ---
        if ser.in_waiting > 0:
            try:
                raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if raw_line == "<TOGGLE>":
                    pyautogui.press('playpause')
                    print("[Action] Play/Pause")
                
                elif raw_line == "<PREV>":
                    pyautogui.press('prevtrack')
                    print("[Action] Previous Track")
                
                elif raw_line == "<NEXT>":
                    pyautogui.press('nexttrack')
                    print("[Action] Next Track")
                
                elif raw_line == "<custom>": #new botton added custo botton 1
                    print("hello mah nigga")
                
                elif raw_line.isdigit():
                    vol_val = int(raw_line)
                    normalized_vol = max(0.0, min(1.0, vol_val / 100.0))
                    if volume_interface:
                        volume_interface.SetMasterVolumeLevelScalar(normalized_vol, None)
                        print(f"[Volume] {vol_val}%")
            except Exception as e:
                print(f"[-] Read Error: {e}")

        # --- 2. HANDLE OUTGOING (PC -> Arduino) ---
        try:
            current_media = await get_media_data()
            if current_media != last_media_data:
                # Wrap in markers < > for the Arduino receiveSerial function
                ser.write(f"<{current_media}>".encode('utf-8'))
                last_media_data = current_media
        except Exception as e:
            print(f"[-] Write Error: {e}")

        # --- 3. YIELD CONTROL ---
        await asyncio.sleep(0.05) # 20Hz update rate is perfect for smooth volume

if __name__ == "__main__":
    try:
        asyncio.run(bridge_loop())
    except KeyboardInterrupt:
        print("\n[!] Bridge stopped by user.")