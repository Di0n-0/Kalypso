# python3 preprocess.py
# import argparse
from PIL import Image
import cv2
import json


def preprocess():
    # ap = argparse.ArgumentParser()
    # ap.add_argument("-d", "--edge-detector", type=str, required=True, help="path to OpenCV's deep learning edge detector")
    # ap.add_argument("-i", "--image", type=str, required=True, help="path to input image")
    # ap.add_argument("-s", "--sensitivity", type=int, required=True, help="the RGB threshold under which the pixels are going to be discarded")
    # args = vars(ap.parse_args())
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
            return [inputs[0][:, :, self.startY : self.endY, self.startX : self.endX]]
    print("[INFO] Loading edge detector...")
    # protoPath = os.path.sep.join([args["edge_detector"], "deploy.prototxt"])
    # modelPath = os.path.sep.join([args["edge_detector"], "hed_pretrained_bsds.caffemodel"])
    protoPath = "res/hed_model/deploy.prototxt"
    modelPath = "res/hed_model/hed_pretrained_bsds.caffemodel"
    net = cv2.dnn.readNetFromCaffe(protoPath, modelPath)
    cv2.dnn_registerLayer("Crop", CropLayer)
    with open("config/settings.json", "r") as json_file:
        data = json.load(json_file)
        filePath = data["filePath"]
        sensivity = data["sensivity"]
    # image = cv2.imread(args["image"])
    image_original = cv2.imread(filePath)
    (H, W) = image_original.shape[:2]
    print("[INFO] performing Canny edge detection...")
    gray = cv2.cvtColor(image_original, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    canny = cv2.Canny(blurred, 30, 150)
    blob = cv2.dnn.blobFromImage(
        image_original,
        scalefactor=1.0,
        size=(W, H),
        mean=(104.00698793, 116.66876762, 122.67891434),
        swapRB=False,
        crop=False,
    )

    print("[INFO] Performing holistically-nested edge detection...")
    net.setInput(blob)
    hed = net.forward()
    hed = cv2.resize(hed[0, 0], (W, H))
    hed = (255 * hed).astype("uint8")
    cv2.imwrite("res/output_hed.png", hed)


    def apply_alpha_threshold(image_path):
        image_temp = Image.open(image_path)
        transparent_image = image_temp.convert("RGBA")
        image_temp.close();
        pixels = transparent_image.load()
        for y in range(transparent_image.size[1]):
            for x in range(transparent_image.size[0]):
                r, g, b, a = pixels[x, y]
                if (
                    not sensivity < r <= 255
                    and not sensivity < g <= 255
                    and not sensivity < b <= 255
                ):
                    pixels[x, y] = (0, 0, 0, 0)
                else:
                    pixels[x, y] = (255, 255, 255, 255)
        transparent_image.save(image_path)


    apply_alpha_threshold("res/output_hed.png")
    print("[INFO] Finding the contours...")
    image_apply_contours = cv2.imread("res/output_hed.png")
    img_gray = cv2.cvtColor(image_apply_contours, cv2.COLOR_BGR2GRAY)
    ret, thresh = cv2.threshold(img_gray, 150, 255, cv2.THRESH_BINARY)
    contours, hierarchy = cv2.findContours(
        image=thresh, mode=cv2.RETR_TREE, method=cv2.CHAIN_APPROX_NONE
    )
    contour_data = []
    for idx, contour in enumerate(contours):
        contour_obj = {"id": idx, "points": contour.tolist()}
        contour_data.append(contour_obj)
    with open("config/settings.json", "w") as json_file:
        data["contour_data"] = contour_data
        json.dump(data, json_file)
    image_copy = image_apply_contours.copy()
    cv2.drawContours(
        image=image_copy,
        contours=contours,
        contourIdx=-1,
        color=(0, 255, 0),
        thickness=2,
        lineType=cv2.LINE_AA,
    )
    cv2.imwrite("res/output_hed_contours.png", image_copy)
    cv2.imwrite("res/original.png", image_original, [cv2.IMWRITE_PNG_COMPRESSION, 9])

    with open("config/settings.json", "w") as json_file:
        data["filePath"] = "res/original.png"
        json.dump(data, json_file)

    print("[INFO] Preprocessing finalized")

if __name__ == "__main__":
    preprocess()
