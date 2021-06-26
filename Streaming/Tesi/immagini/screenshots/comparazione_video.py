from skimage.metrics import structural_similarity
from skimage.metrics import peak_signal_noise_ratio
from skimage.io import imread

def PSNR(D, A, imgD, imgA):
	score = peak_signal_noise_ratio(imgD, imgA)
	print(f"{A} PSNR {score}")
	
def SSIM(D, A, imgD, imgA):
	score = structural_similarity(imgD, imgA, multichannel=True)
	print(f"{A} SSIM {score}")

def main(D, A):
	imgD = imread(D)
	imgA = imread(A)

	PSNR(D, A, imgD, imgA)
	SSIM(D, A, imgD, imgA)

main("comparazioneS.png", "comparazioneW.png")
