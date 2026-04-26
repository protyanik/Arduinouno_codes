import asyncio
import serial
import pyautogui
from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as Manager

PORT = 'COM3'  # Double check your COM port
ser = serial.Serial(PORT, 9600, timeout=0.1) # Added short timeout

def toggle_playback():
    pyautogui.press('playpause')
    print("Toggled!")

async def get_media_data():
    try:
        sessions = await Manager.request_async()
        current_session = sessions.get_current_session()
        
        if current_session:
            info = await current_session.try_get_media_properties_async()
            timeline = current_session.get_timeline_properties()
            
            if info:
                title = info.title
                artist = info.artist
                album = info.album_title if info.album_title else "Single"
                
                duration = timeline.end_time.total_seconds()
                pos = timeline.position.total_seconds()
                percent = int((pos / duration) * 100) if duration > 0 else 0
                    
                # Format: Title - Artist|Album|Percent
                return f"{title} - {artist}|{album}|{percent}"
    except Exception as e:
        print(f"Media Error: {e}")
            
    return "Nothing Playing|Waiting...|0"

async def main():
    last_data = ""
    print("Bridge Started. Listening for Button and Media...")
    
    while True:
        # --- 1. Check for Button Press (Read from Arduino) ---
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8').strip()
                if line == "<TOGGLE>":
                    toggle_playback()
            except Exception as e:
                print(f"Serial Read Error: {e}")

        # --- 2. Check for Media Change (Write to Arduino) ---
        try:
            data = await get_media_data()
            if data != last_data:
                # Wrap in markers < > so Arduino receiveSerial() sees it
                ser.write(f"<{data}>".encode('utf-8'))
                last_data = data
        except Exception as e:
            print(f"Serial Write Error: {e}")

        # --- 3. Yield Control ---
        # A small sleep prevents the CPU from hitting 100% 
        # and lets the button stay responsive.
        await asyncio.sleep(0.1) 

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Stopping...")
        ser.close()