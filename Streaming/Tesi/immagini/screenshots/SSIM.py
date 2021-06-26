from skimage.metrics import structural_similarity
from skimage.metrics import peak_signal_noise_ratio
import argparse
import imutils
import cv2

def PSNR(D, A, imgD, imgA):
	score = peak_signal_noise_ratio(imgD, imgA)
	print(f"{A} PSNR {score}")
	
def SSIM(D, A, imgD, imgA):
	score = structural_similarity(imgD, imgA, multichannel=True)
	print(f"{A} SSIM {score}")

def main(D, A):
	imgD = cv2.imread(D)
	imgA = cv2.imread(A)

	PSNR(D, A, imgD, imgA)
	SSIM(D, A, imgD, imgA)

for x in range(1, 7):
	main(f"{x}s.png", f"{x}w.png")
