from math import log10, sqrt
import cv2
import numpy as np

def PSNR(original, compressed):
	mse = np.mean((original - compressed) ** 2)
	
	if(mse == 0):
		return 100
		
	max_pixel = 255.0
	psnr = 20 * log10(max_pixel / sqrt(mse))
	
	return psnr

def main(D, A):
	original = cv2.imread(D)
	compressed = cv2.imread(A, 1)
	value = PSNR(original, compressed)
	print(f"{A} PSNR {value} dB")

for x in range(1, 7):	
	main(f"{x}s.png", f"{x}w.png")
