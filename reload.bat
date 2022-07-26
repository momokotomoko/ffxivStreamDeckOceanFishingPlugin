taskkill /IM StreamDeck.exe /F
timeout /t 1
call repackage.bat
rmdir /s /q "C:\Users\%USERNAME%\AppData\Roaming\Elgato\StreamDeck\Plugins\com.elgato.ffxivoceanfishing.sdPlugin"
start "" "Release\com.elgato.ffxivoceanfishing.streamDeckPlugin"