#python3 detect_edges_image.py --edge-detector hed_model --image image.jpeg
from PIL import Image
import argparse
import cv2
import numpy as np
import os

ap = argparse.ArgumentParser()
ap.add_argument("-d", "--edge-detector", type=str, required=True, help="path to OpenCV's deep learning edge detector")
ap.add_argument("-i", "--image", type=str, required=True, help="path to input image")
ap.add_argument("-s", "--sensitivity", type=int, required=True, help="the RGB threshold under which the pixels are going to be discarded")
args = vars(ap.parse_args())

class CropLayer(object):
    def __init__(self, params, blobs):
        self.startX = 0
        self.startY = 0
        self.endX = 0
        self.endY = 0

    def getMemoryShapes(self, inputs):
        (inputShape, targetShape) = (inputs[0], inputs[1])
        (batchSize, numChannels) = (inputShape[0], inputShape[1])
        (H, W) = (targetShape[2], targetShape[3])
        self.startX = int((inputShape[3] - targetShape[3]) / 2)
        self.startY = int((inputShape[2] - targetShape[2]) / 2)
        self.endX = self.startX + W
        self.endY = self.startY + H

        return [[batchSize, numChannels, H, W]]

    def forward(self, inputs):
        return [inputs[0][:, :, self.startY:self.endY, self.startX:self.endX]]


print("[INFO] loading edge detector...")
protoPath = os.path.sep.join([args["edge_detector"], "deploy.prototxt"])
modelPath = os.path.sep.join([args["edge_detector"], "hed_pretrained_bsds.caffemodel"])
net = cv2.dnn.readNetFromCaffe(protoPath, modelPath)
cv2.dnn_registerLayer("Crop", CropLayer)


image = cv2.imread(args["image"])
(H, W) = image.shape[:2]

print("[INFO] performing Canny edge detection...")
gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
blurred = cv2.GaussianBlur(gray, (5, 5), 0)
canny = cv2.Canny(blurred, 30, 150)


blob = cv2.dnn.blobFromImage(image, scalefactor=1.0, size=(W, H),
                             mean=(104.00698793, 116.66876762, 122.67891434),
                             swapRB=False, crop=False)

print("[INFO] performing holistically-nested edge detection...")
net.setInput(blob)
hed = net.forward()
hed = cv2.resize(hed[0, 0], (W, H))
hed = (255 * hed).astype("uint8")


cv2.imwrite("output_hed.png", hed)
cv2.imwrite("test.png", hed)

def apply_alpha_threshold(image_path):

    image = Image.open(image_path)


    transparent_image = image.convert("RGBA")
    pixels = transparent_image.load()


    for y in range(transparent_image.size[1]):
        for x in range(transparent_image.size[0]):
            r, g, b, a = pixels[x, y]
            if not args["sensitivity"] < r <= 255 and not args["sensitivity"] < g <= 255 and not args["sensitivity"] < b <= 255:
                pixels[x, y] = (0, 0, 0, 0)
            else: 
                pixels[x,y] = (255, 255, 255, 255)


    transparent_image.save(image_path)


apply_alpha_threshold("output_hed.png")


image = cv2.imread("output_hed.png")

img_gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

ret, thresh = cv2.threshold(img_gray, 150, 255, cv2.THRESH_BINARY)


contours, hierarchy = cv2.findContours(image=thresh, mode=cv2.RETR_TREE, method=cv2.CHAIN_APPROX_NONE)

with open("contours.txt", "w+") as contours_txt:
    for contour in contours:
        contours_txt.write(str(contour) + "\n\n")


image_copy = image.copy()
cv2.drawContours(image=image_copy, contours=contours, contourIdx=-1, color=(0, 255, 0), thickness=2, lineType=cv2.LINE_AA)


cv2.imwrite("output_hed_contours.png", image_copy)

