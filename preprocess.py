import json
import cv2
import numpy as np
import tensorflow as tf
from PIL import Image

def preprocess():
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
            return [inputs[0][:, :, self.startY: self.endY, self.startX: self.endX]]

    print("INFO: Loading edge detector...")
    protoPath = "res/models/deploy.prototxt"
    modelPath = "res/models/hed_pretrained_bsds.caffemodel"
    net = cv2.dnn.readNetFromCaffe(protoPath, modelPath)
    cv2.dnn_registerLayer("Crop", CropLayer)

    with open("config/settings.json", "r") as json_file:
        data = json.load(json_file)
        filePath = data["filePath"]
        sensitivity = data["sensitivity"]

    img_original = cv2.imread(filePath)
    img_replica = img_original.copy()
    cv2.imwrite("res/replica.png", img_replica)
    (H, W) = img_original.shape[:2]

    print("INFO: Performing Canny edge detection...")
    gray = cv2.cvtColor(img_original, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    canny = cv2.Canny(blurred, 30, 150)

    blob = cv2.dnn.blobFromImage(
        img_original,
        scalefactor=1.0,
        size=(W, H),
        mean=(104.00698793, 116.66876762, 122.67891434),
        swapRB=False,
        crop=False,
    )

    print("INFO: Performing holistically-nested edge detection...")
    net.setInput(blob)
    hed = net.forward()
    hed = cv2.resize(hed[0, 0], (W, H))
    hed = (255 * hed).astype("uint8")
    cv2.imwrite("res/output_hed.png", hed)

    def apply_alpha_threshold(image_path):
        image_temp = Image.open(image_path)
        transparent_image = image_temp.convert("RGBA")
        image_temp.close()
        pixels = transparent_image.load()
        for y in range(transparent_image.size[1]):
            for x in range(transparent_image.size[0]):
                r, g, b, a = pixels[x, y]
                if (
                    not sensitivity < r <= 255
                    and not sensitivity < g <= 255
                    and not sensitivity < b <= 255
                ):
                    pixels[x, y] = (0, 0, 0, 0)
                else:
                    pixels[x, y] = (255, 255, 255, 255)
        transparent_image.save(image_path)

    apply_alpha_threshold("res/output_hed.png")
    print("INFO: Finding the contours...")
    img_apply_contours = cv2.imread("res/output_hed.png")
    img_gray = cv2.cvtColor(img_apply_contours, cv2.COLOR_BGR2GRAY)
    ret, thresh = cv2.threshold(img_gray, 150, 255, cv2.THRESH_BINARY)
    contours, hierarchy = cv2.findContours(
        image=thresh, mode=cv2.RETR_TREE, method=cv2.CHAIN_APPROX_NONE
    )
    contour_data = []
    for idx, contour in enumerate(contours):
        contour_obj = {"id_hed": idx, "points_hed": contour.tolist()}
        contour_data.append(contour_obj)
    with open("config/settings.json", "w") as json_file:
        data["contour_data_hed"] = contour_data
        json.dump(data, json_file)
    img_copy = img_apply_contours.copy()
    cv2.drawContours(
        image=img_copy,
        contours=contours,
        contourIdx=-1,
        color=(0, 255, 0),
        thickness=2,
        lineType=cv2.LINE_AA,
    )
    cv2.imwrite("res/output_hed_contours.png", img_copy)

    with open("config/settings.json", "w") as json_file:
        data["filePath"] = "res/replica.png"
        json.dump(data, json_file)

    print("INFO: Applying M-LSD...")
    def pred_lines(
        image,
        interpreter,
        input_details,
        output_details,
        input_shape=[512, 512],
        score_thr=0.10,
        dist_thr=20.0,
    ):
        h, w, _ = image.shape
        h_ratio, w_ratio = [h / input_shape[0], w / input_shape[1]]

        resized_image = np.concatenate(
            [
                cv2.resize(
                    image,
                    (input_shape[0], input_shape[1]),
                    interpolation=cv2.INTER_AREA,
                ),
                np.ones([input_shape[0], input_shape[1], 1]),
            ],
            axis=-1,
        )
        batch_image = np.expand_dims(resized_image, axis=0).astype("float32")
        interpreter.set_tensor(input_details[0]["index"], batch_image)
        interpreter.invoke()

        pts = interpreter.get_tensor(output_details[0]["index"])[0]
        pts_score = interpreter.get_tensor(output_details[1]["index"])[0]
        vmap = interpreter.get_tensor(output_details[2]["index"])[0]

        start = vmap[:, :, :2]
        end = vmap[:, :, 2:]
        dist_map = np.sqrt(np.sum((start - end) ** 2, axis=-1))

        segments_list = []
        for center, score in zip(pts, pts_score):
            y, x = center
            distance = dist_map[y, x]
            if score > score_thr and distance > dist_thr:
                disp_x_start, disp_y_start, disp_x_end, disp_y_end = vmap[y, x, :]
                x_start = x + disp_x_start
                y_start = y + disp_y_start
                x_end = x + disp_x_end
                y_end = y + disp_y_end
                segments_list.append([x_start, y_start, x_end, y_end])

        lines = 2 * np.array(segments_list)  # 256 > 512
        lines[:, 0] = lines[:, 0] * w_ratio
        lines[:, 1] = lines[:, 1] * h_ratio
        lines[:, 2] = lines[:, 2] * w_ratio
        lines[:, 3] = lines[:, 3] * h_ratio

        return lines

    model_name = "res/models/M-LSD_512_large_fp32.tflite"
    interpreter = tf.lite.Interpreter(model_path=model_name)

    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    img_input = cv2.imread(filePath)
    img_blank = np.zeros_like(img_input)
    img_blank.fill(0)

    lines = pred_lines(
        img_input,
        interpreter,
        input_details,
        output_details,
        input_shape=[512, 512],
        score_thr=0.1,
        dist_thr=20,
    )

    for line in lines:
        x_start, y_start, x_end, y_end = [int(val) for val in line]
        cv2.line(img_blank, (x_start, y_start), (x_end, y_end), [0, 255, 255], 2)

    img_gray = cv2.cvtColor(img_blank, cv2.COLOR_BGR2GRAY)
    ret, thresh = cv2.threshold(img_gray, 150, 255, cv2.THRESH_BINARY)

    contours, hierarchy = cv2.findContours(
        image=thresh, mode=cv2.RETR_TREE, method=cv2.CHAIN_APPROX_NONE
    )
    contour_data = []

    for idx, contour in enumerate(contours):
        contour_obj = {"id_m-lsd": idx, "points_m-lsd": contour.tolist()}
        contour_data.append(contour_obj)

    with open("config/settings.json", "r") as json_file:
        existing_data = json.load(json_file)

    existing_data["contour_data_m-lsd"] = contour_data

    with open("config/settings.json", "w") as json_file:
        json.dump(existing_data, json_file)

    img_output = img_input.copy()
    cv2.drawContours(
        image=img_output,
        contours=contours,
        contourIdx=-1,
        color=(0, 255, 0),
        thickness=2,
        lineType=cv2.LINE_AA,
    )

    cv2.imwrite("res/output_m-lsd-contours.png", img_output)

    print("INFO: Preprocessing finalized")


if __name__ == "__main__":
    preprocess()
