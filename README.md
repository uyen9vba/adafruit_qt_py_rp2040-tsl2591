# adafruit_qt_py_rp2040-tsl2591

```
west build -b adafruit_qt_py_rp2040
cp build/zephyr/zephyr.uf2 mnt
nohup pythonw scripts/host.py &>/dev/null &
# current jobs in session
jobs
```

## Windows
The script uses home path as default location. Add batch file under startup programs.
```
cp scripts/adafruit_qt_py_rp2040-tsl2591-startup.bat ~/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup
```

## TODO
Low pass filter or something similar
Linux startup script 
Control more than 1 monitor simultaneously (config script?)

