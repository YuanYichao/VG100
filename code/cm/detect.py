import time, sensor, image, pyb
from image import SEARCH_EX, SEARCH_DS

sensor.reset()
sensor.set_contrast(1)
sensor.set_gainceiling(16)
sensor.set_framesize(sensor.VGA)
#we only need gray image
sensor.set_pixformat(sensor.GRAYSCALE)
#begin the serial
uart = pyb.UART(3, 9600, timeout_char = 1000)

#magic numbers
test_threshold = (0,70)
binary_threshold = (100, 255)

arrowRatio = 557 /296

right = 0
left = 0
lenState = 0
record = 0

def available():
    return uart.any()

def getCommand():
    mstr = ""
    if available():
        c = str(chr(uart.readchar()))
        print(c)
        mstr += c
        while c != ";":
            if available():
                c = str(chr(uart.readchar()))
                print(c)
                mstr += c
    print(mstr)
    return mstr[:-1]

def startRecord():
    global right
    global left
    global record
    right = 0
    left = 0
    record = 1

def endRecord():
    global right
    global left
    global record
    right = 0
    left = 0
    record = 0

def getDir():
    global right
    global left
    print(right)
    print(left)
    if right > left:
        uart.write('r')
    elif left > right:
        uart.write('l')
    else:
        uart.write('u')

def cdLenState():
    global lenState
    global sensor
    if lenState:
        lenState = 0
        sensor.set_windowing(((640 - 160)//2, (480 - 120)//2, 160, 120))
    else:
        lenState = 1
        sensor.set_windowing(((640 - 320)//2, (480 - 240)//2, 320, 240))

operation = { "start": startRecord, "end": endRecord, "dir": getDir, "cd": cdLenState}


def testRatio(wi, he):
    err = (wi/he - arrowRatio)/(wi/he)
    if err > 0.10 or err <- 0.10:
        return 0
    return 1

def testDensity(d):
    if d > 0.9:
        return 0
    return 1

#0 -> left 1 -> right
def judgeDir(img, a):
   sl = img.get_statistics(roi = (a.x() + 3 * a.w()//10 ,a.y(), a.w()//5,a.h()))
   sr = img.get_statistics(roi = (a.x() + a.w()//2, a.y(), a.w()//5,a.h()))
   print("left is %d" % sl.mean())
   print("right is %d" % sr.mean())
   if sl.mean() < sr.mean():
       return 0
   return 1

#0-> left 1->right
def detectDir():
    img = sensor.snapshot()
    img.binary([binary_threshold])
    b = img.find_blobs([test_threshold], area_threshold=400,
                       roi=(0, (480-300)//2, 640, 300))
    res = []
    if b:
        for a in b:
            if testRatio(a.w(), a.h()) and testDensity(a.density()):
                res.append(a)
        if len(res) == 1:
            for a in res:
                img.draw_rectangle(a[0], a[1], a[2], a[3], color=(0, 200, 0))
                img.draw_cross(a[5], a[6])
                if judgeDir(img, a):
                    print("right")
                    return 1
                else:
                    print("left")
                    return 0

while (True):
    if available():
        mstr = getCommand()
        print(mstr)
        operation[mstr]()
    if record:
        lr = detectDir()
        if lr:
            right += 1
        else:
            left += 1