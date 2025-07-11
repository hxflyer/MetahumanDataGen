import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from PIL import Image
import cv2

def load_and_process_depth_png(file_path):
    """
    Load PNG depth image and convert using R + G/255 + B/65025 formula
    """
    # Load the image
    img = Image.open(file_path)
    img_array = np.array(img)
    
    print(f"Image shape: {img_array.shape}")
    print(f"Image dtype: {img_array.dtype}")
    
    # Extract RGB channels
    if len(img_array.shape) == 3:
        R = img_array[:, :, 0].astype(np.float32)
        G = img_array[:, :, 1].astype(np.float32)
        B = img_array[:, :, 2].astype(np.float32)
    else:
        # Grayscale image
        R = img_array.astype(np.float32)
        G = np.zeros_like(R)
        B = np.zeros_like(R)
    
    
    depth =(R*65025.0  + G*255.0+ B)/255
    
    white_mask = (R == 255) & (G == 255) & (B == 255)
    
    # Option 1: Set to 0 (no depth/background)
    depth[white_mask] = 0.0
    

    print(f"Depth range: {depth.min()} to {depth.max()}")
    
    return depth, img_array

def create_point_cloud_3d(depth_map, subsample_factor=2):
    """
    Create and visualize point cloud from depth map
    """
    height, width = depth_map.shape
    
    # Create coordinate arrays
    y_coords, x_coords = np.mgrid[0:height:subsample_factor, 0:width:subsample_factor]
    z_coords = depth_map[::subsample_factor, ::subsample_factor]
    
    # Flatten arrays
    x_flat = x_coords.flatten()
    y_flat = y_coords.flatten()
    z_flat = z_coords.flatten()
    
    # Remove invalid depth values (if any)
    valid_mask = (z_flat > 0) & (z_flat < np.inf) & (~np.isnan(z_flat))
    x_valid = x_flat[valid_mask]
    y_valid = y_flat[valid_mask]
    z_valid = z_flat[valid_mask]
    
    # Create 3D scatter plot
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    # Color points by depth
    scatter = ax.scatter(x_valid, y_valid, z_valid, 
                        c=z_valid, cmap='plasma', 
                        s=1, alpha=0.6)
    
    ax.set_title('3D Point Cloud from Depth Map')
    ax.set_xlabel('X (pixels)')
    ax.set_ylabel('Y (pixels)')
    ax.set_zlabel('Depth')
    
    fig.colorbar(scatter, ax=ax, shrink=0.5, label='Depth')
    plt.show()

# Main execution
if __name__ == "__main__":
    # Replace with your PNG file path
    png_file_path = "../Saved/CapturedFrames/Depth_000100.bmp"  # Change this to your file path
    
    depth_map, original_img = load_and_process_depth_png(png_file_path)
    create_point_cloud_3d(depth_map, subsample_factor=4)
        