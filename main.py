import os
from src.preprocess import preprocess_image

def run_pipeline(image_path: str, output_dir: str = "output"):
    print("Preprocessing image...")
    preprocess_image(image_path, output_dir=output_dir, save_steps=True)

if __name__ == "__main__":
    imgs = os.listdir("imgs")
    for img in imgs:
        if img.lower().endswith(('.jpg', '.jpeg', '.png', '.webp', '.tiff', '.tif')):
            image_path = os.path.join("imgs", img)
            output_dir = os.path.join("output", os.path.splitext(img)[0])
            run_pipeline(image_path, output_dir=output_dir)