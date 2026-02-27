"""
preprocess.py

Purpose:
Load image and perform preprocessing to prepare for feature extraction.

Outputs cleaned images for downstream processing.
"""
from PIL import Image           # Used for loading image safely and format handling
import cv2                     # OpenCV for computer vision operations
import numpy as np            # Numerical operations and array handling
import os


def load_image(image_path: str) -> np.ndarray:
    """
    Load image from disk using PIL and convert to OpenCV format.

    Args:
        image_path: Path to input image

    Returns:
        OpenCV image (numpy array)
    """
    # Load using PIL first (better format support than cv2.imread)
    pil_image = Image.open(image_path).convert("RGB")

    # Convert PIL image → NumPy array → OpenCV format
    # PIL uses RGB
    # OpenCV uses BGR
    cv_image = cv2.cvtColor(
        np.array(pil_image),
        cv2.COLOR_RGB2BGR
    )
    return cv_image

def convert_to_grayscale(cv_image: np.ndarray) -> np.ndarray:
    """
    Convert color image to grayscale.

    Args:
        cv_image: input color image in OpenCV format

    Returns:
        np.ndarray: grayscale image (numpy array)
    """
    gray_image = cv2.cvtColor(
        cv_image,
        cv2.COLOR_BGR2GRAY
    )
    return gray_image

def remove_noise(gray_image: np.ndarray) -> np.ndarray:
    """
    Smoothen noise.

    Args:
        gray_image (np.ndarray): grayscale image

    Returns:
        np.ndarray: blurred image
    """
    # Gaussian blur smooths noise while preserving structure
    # Kernel size (5,5) = standard starting point
    # Larger kernel = more smoothing but more information loss
    blur_image = cv2.GaussianBlur(
        gray_image,
        ksize=(5, 5),
        sigmaX=0
    )
    # Alternative option:
    # blur = cv2.medianBlur(gray, 5)
    # Median blur is better for salt-and-pepper noise
    return blur_image

def threshold_image(blur_image: np.ndarray, invert: bool = False) -> np.ndarray:
    """
    Threshold image to binary (black and white).

    Args:
        blur_image (np.ndarray): blurred grayscale image
        invert: if True, black and white are inverted (black=255, white=0)

    Returns:
        np.ndarray: thresholded binary image
    """
    # Automatically select threshold so that black pixel coverage is between 5% and 25%
    desired_min = 0.05
    desired_max = 0.25
    best_thresh = 200
    best_coverage = 0.0
    found = False
    for thresh in range(255, 0, -5):
        _, temp = cv2.threshold(
            src=blur_image,
            thresh=thresh,
            maxval=255,
            type=cv2.THRESH_BINARY_INV if invert else cv2.THRESH_BINARY
        )
        black_pixels = np.sum(temp == 255)  # Inverted: black is 255
        total_pixels = temp.size
        coverage = black_pixels / total_pixels
        if desired_min <= coverage <= desired_max:
            best_thresh = thresh
            best_coverage = coverage
            found = True
            break
        # Track closest coverage if not found
        if not found and abs(coverage - desired_min) < abs(best_coverage - desired_min):
            best_thresh = thresh
            best_coverage = coverage
    
    _, thresh_image = cv2.threshold(
        src=blur_image,
        thresh=best_thresh,
        maxval=255,
        type=cv2.THRESH_BINARY_INV if invert else cv2.THRESH_BINARY
    )
    return thresh_image

def close_gaps(thresh_image: np.ndarray) -> np.ndarray:
    """
    Close small gaps in contours to connect broken lines.

    Args:
        thresh_image (np.ndarray): thresholded binary image

    Returns:
        np.ndarray: image with closed contours (small gaps filled)
    """
    # Closing fills small gaps and connects broken lines
    kernel = np.ones((3, 3), np.uint8)
    closed_image = cv2.morphologyEx(
        src=thresh_image,
        op=cv2.MORPH_CLOSE,
        kernel=kernel
    )
    return closed_image

def filter_primary_component(binary_image: np.ndarray,
                             min_area_ratio: float = 0.01) -> np.ndarray:
    """
    Keep only the largest connected component.

    This removes annotation boxes, leader lines, and text.

    Args:
        binary_image: closed binary image
        min_area_ratio: minimum relative area threshold (safety filter)

    Returns:
        np.ndarray: filtered binary image
    """

    # Label all connected components
    num_labels, labels, stats, _ = cv2.connectedComponentsWithStats(
        binary_image,
        connectivity=8
    )

    # stats format:
    # stats[i] = [x, y, width, height, area]

    image_area = binary_image.shape[0] * binary_image.shape[1]

    # Ignore label 0 (background)
    component_areas = stats[1:, cv2.CC_STAT_AREA]

    if len(component_areas) == 0:
        return binary_image

    # Find largest component
    largest_index = np.argmax(component_areas) + 1

    largest_area = component_areas[np.argmax(component_areas)]

    # Safety check to avoid keeping tiny noise
    if largest_area < image_area * min_area_ratio:
        return binary_image

    # Create empty output image
    filtered_image = np.zeros_like(binary_image)

    # Keep only largest component
    filtered_image[labels == largest_index] = 255

    return filtered_image

def detect_edges(binary_image: np.ndarray) -> np.ndarray:
    """
    Edge detection.

    Args:
        binary_image (np.ndarray): thresholded binary image

    Returns:
        np.ndarray: edge-detected binary image
    """
    # Canny detects edges based on gradient change
    edges = cv2.Canny(
        image=binary_image,
        threshold1=50,
        threshold2=150
    )
    return edges

def save_intermediates(output_dir: str,
                       gray: np.ndarray,
                       blur: np.ndarray,
                       thresh: np.ndarray,
                       closing: np.ndarray,
                       filtered: np.ndarray,
                       edges: np.ndarray):

    os.makedirs(output_dir, exist_ok=True)

    cv2.imwrite(f"{output_dir}/gray.png", gray)
    cv2.imwrite(f"{output_dir}/blur.png", blur)
    cv2.imwrite(f"{output_dir}/threshold.png", thresh)
    cv2.imwrite(f"{output_dir}/closing.png", closing)
    cv2.imwrite(f"{output_dir}/processed.png", filtered)
    cv2.imwrite(f"{output_dir}/edges.png", edges)

def preprocess_image(image_path: str,
                     output_dir: str = "output",
                     save_steps: bool = False) -> dict:

    cv_image = load_image(image_path)
    gray = convert_to_grayscale(cv_image)
    blur = remove_noise(gray)
    thresh = threshold_image(blur, invert=True)
    closing = close_gaps(thresh)
    filtered = filter_primary_component(closing)
    edges = detect_edges(filtered)
    
    if save_steps:
        save_intermediates(
            output_dir,
            gray,
            blur,
            thresh,
            closing,
            filtered,
            edges
        )

    return {
        "processed": filtered,
        "edges": edges
    }