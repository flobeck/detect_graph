import cv2
import numpy as np
from matplotlib import pyplot as plt

img = cv2.imread('../data/K4.jpg')
color = ('b','g','r')

(H,W,_) = img.shape
D = W*H
print "W: ", W
print "H: ", H

def normalize(bin):
    return bin*(1.0/D)

for i,col in enumerate(color):
    histr = cv2.calcHist([img],[i],None,[256],[0,256])
    histr = [map(normalize, h) for h in histr]
    plt.plot(histr,color = col)
    plt.xlim([0,256])
plt.show()