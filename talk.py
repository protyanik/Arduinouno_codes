import asyncio
import serial
import time
from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as Manager

PORT = 'COM3' # Change to your COM port
ser = serial.Serial(PORT, 9600, timeout=1)

async def get_media_data():
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
                
            # Formatting: Title - Artist | Album | Percent
            return f"{title} - {artist}|{album}|{percent}"
            
    return "Nothing Playing|Waiting...|0"

async def main():
    last_data = ""
    while True:
        try:
            data = await get_media_data()
            if data != last_data:
                ser.write(f"<{data}>".encode('utf-8'))
                last_data = data
        except Exception:
            pass
        await asyncio.sleep(1)

if __name__ == "__main__":
    asyncio.run(main())