import time, sensor, image, pyb
from image import SEARCH_EX, SEARCH_DS

sensor.reset()
sensor.set_contrast(1)
sensor.set_gainceiling(16)
sensor.set_framesize(sensor.VGA)
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_windowing(((640 - 160)//2, (480 - 120)//2, 160, 120))
uart = pyb.UART(3, 9600, timeout_char = 1000)
templater = image.Image("/arrowr.pgm")
templatel = image.Image("/arrowl.pgm")
binary_threshold = (235, 255)

# Run template matching

def available():
    return uart.any()

def getCommand():
    mstr = ""
    if available():
        while uart.any():
            c = str(chr(uart.readchar()))
            print(c)
            if c == ";":
                break
                print(c)
            mstr += c
    print(mstr)
    return mstr

right = 0;
left = 0;
lenState = 0;
record = 0;

def startRecord():
    global right
    global left
    global record
    right = 0
    left = 0
    lenState = 0
    record = 1

def endRecord():
    global right
    global left
    global record
    right = 0
    left = 0
    lenState = 0
    record = 0

def getDir():
    global right
    global left
    print(right)
    print(left)
    if right > left:
        uart.write('r')
        print("herer")
    elif left > right:
        uart.write('l')
        print("herel")
    else:
        uart.write('u')
        print("hereu")

def cdLenState():
    global lenState
    global sensor
    if lenState:
        lenState = 0
        sensor.set_windowing(((640 - 160)//2, (480 - 120)//2, 160, 120))
    else:
        lenState = 1
        sensor.set_windowing(((640 - 320)//2, (480 - 240)//2, 320, 240))

def correct(img):
    img.histeq(adaptive=True, clip_limit=3)
    img.binary([binary_threshold])
    img.laplacian(1, sharpen=True)
    img.lens_corr(strength = 1, zoom = 1.0)
    return img

operation = { "start": startRecord, "end": endRecord, "dir": getDir, "cd": cdLenState}

while (True):
    if available():
        mstr = getCommand()
        print(mstr)
        operation[mstr]()
    img = sensor.snapshot()
    img = correct(img)
    if record:
        r = img.find_template(templater, 0.70, roi = (0,0, img.width(), img.height()), step=4, search=SEARCH_DS)
        l = img.find_template(templatel, 0.70, roi = (0,0, img.width(), img.height()), step=4, search=SEARCH_DS)
        if r:
            img.draw_rectangle(r)
            right = right + 1
            print("right")
        if l:
            img.draw_rectangle(l)
            left = left + 1
            print("left")
